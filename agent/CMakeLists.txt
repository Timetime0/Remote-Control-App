cmake_minimum_required(VERSION 4.1)
project(remote_control_agent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(remote_agent
  src/main.cpp
)

if(WIN32)
  target_link_libraries(remote_agent ws2_32)
endif()

if(APPLE)
  target_link_libraries(remote_agent "-framework ApplicationServices")
endif()
