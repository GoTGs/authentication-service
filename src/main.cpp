#include "../include/endpoints.hpp"
#include <thread>

int main(int argc, char** argv) {
	CppHttp::Net::Router router;
	CppHttp::Net::TcpListener server;
	server.CreateSocket();

	int requestCount = 0;

	auto onReceive = [&](CppHttp::Net::Request req) {
		router.Handle(req);
	};

	server.SetOnReceive(onReceive);

	router.AddRoute("POST", "/register", Register);
	router.AddRoute("POST", "/login", Login);

	server.Listen("0.0.0.0", 8000, std::thread::hardware_concurrency());
}