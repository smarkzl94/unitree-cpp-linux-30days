cmake_minimum_required(VERSION 3.16)
project(telemetry_hub LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(hub_server
  src/hub_server.cpp
)
target_include_directories(hub_server PRIVATE include)
target_link_libraries(hub_server PRIVATE Threads::Threads)

add_executable(hub_client
  src/hub_client.cpp
)
target_include_directories(hub_client PRIVATE include)
