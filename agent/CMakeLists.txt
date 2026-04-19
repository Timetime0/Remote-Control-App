cmake_minimum_required(VERSION 3.10)
project(remote_control_agent LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(remote_agent
    src/main.cpp
    src/commands.cpp
    src/utils.cpp
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
  target_link_libraries(remote_agent ws2_32 user32)
endif()

if(APPLE)
  target_link_libraries(remote_agent "-framework ApplicationServices")
endif()

# --- OpenCV (webcam) — đường dẫn trên máy bạn ---------------------------------
# Thư mục phải chứa OpenCVConfig.cmake (thường là .../build, .../install/lib/cmake/opencv4).
#
# Cách 1 — file cục bộ (khuyến nghị): copy OpenCVPath.cmake.example -> OpenCVPath.cmake,
#         sửa đường dẫn trong OpenCVPath.cmake (file này được .gitignore, không đẩy lên git).
#
# Cách 2 — Visual Studio Code (CMake Tools): tạo .vscode/settings.json ở root repo với:
#         "cmake.configureSettings": { "OpenCV_DIR": "C:/path/to/opencv/build" }
#         hoặc "cmake.configureArgs": ["-DOpenCV_DIR=C:/path/to/opencv/build"]
#
# Cách 3 — dòng lệnh: cmake -S agent -B build -DOpenCV_DIR=C:/path/to/opencv/build
#
include("${CMAKE_CURRENT_LIST_DIR}/OpenCVPath.cmake" OPTIONAL)

find_package(OpenCV QUIET COMPONENTS core videoio imgcodecs imgproc)
if(OpenCV_FOUND)
  message(STATUS "OpenCV ${OpenCV_VERSION} — webcam enabled")
  target_compile_definitions(remote_agent PRIVATE HAVE_OPENCV)
  target_include_directories(remote_agent PRIVATE ${OpenCV_INCLUDE_DIRS})
  target_link_libraries(remote_agent ${OpenCV_LIBS})

  # Windows + OpenCV động (vcpkg): copy opencv*.dll cạnh exe để chạy không cần cài OpenCV.
  # Cách khác: build triplet x64-windows-static để nhét OpenCV vào exe (file to hơn).
  if(WIN32)
    get_filename_component(_opencv_triplet_root "${OpenCV_DIR}/../.." ABSOLUTE)
    set(_opencv_bin "${_opencv_triplet_root}/bin")
    if(EXISTS "${_opencv_bin}")
      file(GLOB _opencv_dlls "${_opencv_bin}/opencv*.dll")
      foreach(dll ${_opencv_dlls})
        add_custom_command(
          TARGET remote_agent
          POST_BUILD
          COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:remote_agent>"
          COMMENT "Copy OpenCV runtime: ${dll}"
        )
      endforeach()
      if(NOT _opencv_dlls)
        message(STATUS "No opencv*.dll in ${_opencv_bin} — nếu webcam lỗi thiếu DLL, copy thủ công từ thư mục bin của triplet vcpkg.")
      endif()
    endif()
  endif()
else()
  message(STATUS "OpenCV not found — build succeeds but webcam needs OpenCV (brew install opencv / vcpkg)")
endif()
