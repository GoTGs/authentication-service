cmake_minimum_required(VERSION 3.13)
project("service")

find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

include_directories(${PostgreSQL_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/cpphttp/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/soci/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

file (GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/cpphttp/src/request.cpp")

add_executable(auth ${SOURCES})

# target_sources(service 
#                       PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/database.cpp"
#                       PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/hash.cpp"
#                       PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/endpoints.cpp")

set_target_properties(auth PROPERTIES CXX_STANDARD 20)

target_link_libraries(auth 
  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/soci/lib/libsoci_postgresql.a
  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/soci/lib/libsoci_core.a
  ${PostgreSQL_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)

set_property(TARGET auth PROPERTY CXX_STANDARD 20)