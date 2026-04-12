cmake_minimum_required(VERSION 3.10)
project(remote_control_agent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(remote_agent
    src/main.cpp
    src/commands.cpp
    src/screenshot.cpp
    src/app_manager.cpp
    src/ping.cpp
    src/process_manager.cpp
    src/system_power.cpp
    src/file_ops.cpp
    src/keylogger.cpp
    src/webcam.cpp
    src/mouse_control.cpp
    src/screen_viewer.cpp
)

if(WIN32)
  target_link_libraries(remote_agent ws2_32)
endif()

if(APPLE)
  target_link_libraries(remote_agent "-framework ApplicationServices")
endif()
