cmake_minimum_required(VERSION 3.16)                      # yêu cầu máy có CMake từ 3.16
project(minishell VERSION 1.0.0 LANGUAGES CXX)            # tên dự án minishell, dùng C++

set(CMAKE_CXX_STANDARD 17)                                # dùng C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)                       # bắt buộc phải có C++17

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "" FORCE)  # cài vào minishell/install/, không ghi ra ngoài dự án

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}") # file chạy minishell nằm ngay trong build/

include_directories(include)                              # cho phép #include "minishell/core/shell.h"
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)    # tìm mọi file .cpp trong src/ (cả thư mục con), tự kiểm tra lại khi thêm/xóa file

add_executable(minishell ${SOURCES})                      # tạo chương trình minishell từ SOURCES
target_compile_options(minishell PRIVATE -Wall -Wextra)   # bật cảnh báo biên dịch

install(TARGETS minishell RUNTIME DESTINATION .)          # chép file chạy vào install/