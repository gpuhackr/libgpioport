cmake_minimum_required(VERSION 2.6)
project(libgpioport)


set(CMAKE_C_FLAGS "-std=c99")

add_executable(test test.c)
target_link_libraries(test pciaccess)
