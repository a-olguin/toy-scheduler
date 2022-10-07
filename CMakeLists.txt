cmake_minimum_required(VERSION 3.22)
project(scheduler)

add_executable(scheduler
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

#target_link_libraries(scheduler
#    /usr/lib/python3.8/config-3.8-x86_64-linux-gnu/libpython3.8.so
#)