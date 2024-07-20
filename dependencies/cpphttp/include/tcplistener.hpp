#include "debug.hpp"
#include "event.hpp"
#include "router.hpp"
#include <iostream>
#include <functional>
#include <thread>
#include <stdexcept>
#include <string>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <syncstream>

#ifdef _WIN32 || _WIN64 || _MSC_VER
    #define WIN32_LEAN_AND_MEAN
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #define WINDOWS
    #include <Windows.h>
    #include <WinSock2.h>
    #include <WS2tcpip.h>

    #pragma comment (lib, "Ws2_32.lib")
#elif defined(__linux__) || defined(__APPLE__)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define LINUX
    #define closesocket close
	#define ioctlsocket ioctl
#endif

namespace CppHttp {
    namespace Net {
        class TcpListener {
        public:
            TcpListener() {
                #ifdef WINDOWS
                    this->InitWSA();
                #endif
            }
            ~TcpListener() {
                this->Close();
            }

            void CreateSocket() {
                this->listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                if (this->listener == INVALID_SOCKET) {
                    std::cout << "\033[31m[-] Failed to create socket...\033[0m\n";

                    #ifdef WINDOWS
                        std::cout << "\033[31m[-] WSA error code: " << WSAGetLastError() << "\033[0m\n";
                    #elif defined(LINUX)
						std::cout << "\033[31m[-] Error code: " << errno << "\033[0m\n";
                        std::cout << "\033[31m[-] Error message: " << strerror(errno) << "\033[0m\n";
                    #endif

                    throw std::runtime_error("Failed to create socket");
                }

                std::cout << "\033[1;32m[+] Created socket\033[0m\n";
            }

            void Listen(const char* ip, uint_fast16_t port, uint_fast8_t maxConnections) {
                this->InitThreadPool(maxConnections);

                this->server.sin_family = AF_INET;

                #ifdef WINDOWS
                    this->server.sin_addr.S_un.S_addr = inet_addr(ip);
                #elif defined(LINUX)
                    this->server.sin_addr.s_addr = inet_addr(ip);
                #endif

                this->server.sin_port = htons(port);
                this->serverLen = sizeof(this->server);
                if (bind(this->listener, (struct sockaddr*)&this->server, this->serverLen) != 0) {
                    std::cout << "\033[31m[-] Failed to bind socket...\033[0m\n";
                    
                    #ifdef WINDOWS
                        std::cout << "\033[31m[-] WSA error code: " << WSAGetLastError() << "\033[0m\n";
                    #elif defined(LINUX)
                        std::cout << "\033[31m[-] Error code: " << errno << "\033[0m\n";
                        std::cout << "\033[31m[-] Error message: " << strerror(errno) << "\033[0m\n";
                    #endif
                    throw std::runtime_error("Failed to bind socket");
                }
                std::cout << "\033[1;32m[+] Bound socket\033[0m\n";

                int backlog = 20;
                if (listen(this->listener, maxConnections) != 0) {
                    std::cout << "\033[31m[-] Failed to listen...\033[0m\n"; 
                    #ifdef WINDOWS
                        std::cout << "\033[31m[-] WSA error code: " << WSAGetLastError() << "\033[0m\n";
                    #elif defined(LINUX)
                        std::cout << "\033[31m[-] Error code: " << errno << "\033[0m\n";
                        std::cout << "\033[31m[-] Error message: " << strerror(errno) << "\033[0m\n";
                    #endif

                    throw std::runtime_error("Failed to listen");
                }
                std::cout << "\033[1;32m[+] Started listening on " << ip << ':' << port << " with " << (int)maxConnections << " max connections\033[0m\n";


                while (true) {
                    try {
                        this->Accept();
                    }
                    catch (std::runtime_error& e) {
                        std::cout << "\033[31m[-] Error: " << e.what() << "\033[0m\n";
                    }
                }
                this->Close();
            }

            void Close() {

                closesocket(this->listener);

                #ifdef WINDOWS
                    WSACleanup();
                #endif
            }

            void SetOnConnect(std::function<void(SOCKET)> callback) {
                this->onConnect.Attach(callback);
            }

            void SetOnDisconnect(std::function<void(SOCKET)> callback) {
                this->onDisconnect.Attach(callback);
            }

            void SetOnReceive(std::function<void(Request&)> callback) {
                this->onReceive.Attach(callback);
            }

            void SetBlocking(bool blocking) {
                #ifdef WINDOWS
                    u_long mode = blocking ? 0 : 1;
                    ioctlsocket(this->listener, FIONBIO, &mode);
                #elif defined(LINUX)
                    int flags = fcntl(this->listener, F_GETFL, 0);
                    if (blocking)
                        flags &= ~O_NONBLOCK;
                    else
                        flags |= O_NONBLOCK;
                    fcntl(this->listener, F_SETFL, flags);
                #endif
            }


        private:
            SOCKET listener = INVALID_SOCKET;
            sockaddr_in server;

            #ifdef WINDOWS
                WSADATA wsaData;
            #endif

            //Request req = Request();

            int serverLen;

            Event<void, SOCKET> onConnect;
            Event<void, SOCKET> onDisconnect;
            Event<void, Request&> onReceive;

            std::vector<std::future<void>> futures;
            std::queue<std::function<void()>> tasks;
            std::vector<std::thread> threads;
            std::mutex queueMutex;
            std::condition_variable condition;

            void InitThreadPool(const int maxConnections) {
                for (int i = 0; i < maxConnections; ++i) {
                    threads.emplace_back([this]() {
                        while (true) {
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(queueMutex);
                                condition.wait(lock, [this]() { return !tasks.empty(); });
                                task = std::move(tasks.front());
                                tasks.pop();
                            }
                            task();
                        }
                    });
                }
            }

            void Accept() {
                #ifdef WINDOWS
                    SOCKET newConnection = accept(listener, (SOCKADDR*)&this->server, &this->serverLen);
                #elif defined(LINUX)
                    SOCKET newConnection = accept(listener, (struct sockaddr*)&this->server, (socklen_t*)&this->serverLen);
                #endif

                if (newConnection == INVALID_SOCKET) {
                    std::osyncstream(std::osyncstream(std::cout)) << "\033[31m[-] Failed to accept new connection...\033[0m\n"; 
                    #ifdef WINDOWS
                        std::osyncstream(std::cout) << "\033[31m[-] WSA error code: " << WSAGetLastError() << "\033[0m\n";
                    #elif defined(LINUX)
                        std::osyncstream(std::cout) << "\033[31m[-] Error code: " << errno << "\033[0m\n";
                        std::osyncstream(std::cout) << "\033[31m[-] Error message: " << strerror(errno) << "\033[0m\n";
                    #endif
                }

                std::unique_lock<std::mutex> lock(queueMutex);
                tasks.push([this, newConnection]() {
                    std::osyncstream(std::osyncstream(std::cout)) << "\033[1;32m[+] Accepted new connection...\033[0m\n";
                    this->onConnect.Invoke(newConnection);

                    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(4096);

                    memset(buffer.get(), 0, 4096);

                    size_t bytesReceived = recv(newConnection, buffer.get(), 4096, 0);

                    if (bytesReceived < 0) {
                        std::osyncstream(std::cout) << "\033[31m[-] Failed to read client request\033[0m\n"; 
                        #ifdef WINDOWS
                            std::osyncstream(std::cout) << "\033[31m[-] WSA error code: " << WSAGetLastError() << "\033[0m\n";
                        #elif defined(LINUX)
                            std::osyncstream(std::cout) << "\033[31m[-] Error code: " << errno << "\033[0m\n";
                            std::osyncstream(std::cout) << "\033[31m[-] Error message: " << strerror(errno) << "\033[0m\n";
                        #endif
                        closesocket(newConnection);
                    }
                    else if (bytesReceived == 0) {
						std::osyncstream(std::cout) << "\033[31m[-] Client disconnected\033[0m\n";
						closesocket(newConnection);

                        #ifdef WINDOWS
                            WSACleanup();
                        #endif

						return;
                    }

                    std::osyncstream(std::cout) << "\033[1;32m[+] Received client request\033[0m\n";

                    std::string data = "";
                    data = std::string(buffer.get());

                    if (data == "") {
                        std::osyncstream(std::cout) << "\033[31m[-] Failed to read client request\033[0m\n";
                        #ifdef WINDOWS
                            std::osyncstream(std::cout) << "\033[31m[-] WSA error code: " << WSAGetLastError() << "\033[0m\n";
                        #elif defined(LINUX)
                            std::osyncstream(std::cout) << "\033[31m[-] Error code: " << errno << "\033[0m\n";
                            std::osyncstream(std::cout) << "\033[31m[-] Error message: " << strerror(errno) << "\033[0m\n";
                        #endif
                        closesocket(newConnection);
                    }

                    #ifdef API_DEBUG
                        std::osyncstream(std::cout) << "\033[1;34m[*] Request data:\n";
                        std::vector<std::string> split = CppHttp::Utils::Split(data, '\n');
                        for (int i = 0; i < split.size(); ++i) {
                            std::osyncstream(std::cout) << "    " << split[i] << '\n';
                        }
                        std::osyncstream(std::cout) << "\033[0m";
                    #endif

                    Request req = Request(data, newConnection);
                    this->onReceive.Invoke(req);

                    this->onDisconnect.Invoke(newConnection);
                    closesocket(newConnection);
                });

                lock.unlock();
                condition.notify_one();
            }

            #ifdef WINDOWS
                void InitWSA() {
                    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                        std::cout << "\033[31m[-] Failed to initialise WSA...\033[0m\n";
                        std::cout << "\033[31m[-] Error code: " << WSAGetLastError() << "\033[0m\n";
                        throw std::runtime_error("Failed to initialise WSA");
                    }
                    std::cout << "\033[1;32m[+] Initialised WSA\033[0m\n";
                }
            #endif
        };
    };
};