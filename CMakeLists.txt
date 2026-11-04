cmake_minimum_required(VERSION 3.16)
project(minishell VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "" FORCE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

file(GLOB_RECURSE MINISHELL_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(minishell ${MINISHELL_SOURCES})
target_include_directories(minishell PRIVATE include)
target_compile_options(minishell PRIVATE -Wall -Wextra)
install(TARGETS minishell RUNTIME DESTINATION .)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_executable(test_parser tests/unit/test_parser.cpp src/core/parser.cpp)
    target_include_directories(test_parser PRIVATE include)
    add_test(NAME parser COMMAND test_parser)
    add_test(
        NAME shell_integration
        COMMAND bash "${CMAKE_SOURCE_DIR}/tests/run_all.sh" "$<TARGET_FILE:minishell>"
    )
    add_test(
        NAME ctrl_c
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tests/test_ctrl_c.py"
                "$<TARGET_FILE:minishell>"
    )
endif()
