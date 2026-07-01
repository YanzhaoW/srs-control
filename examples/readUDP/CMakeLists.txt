cmake_minimum_required(VERSION 3.22)
project(readUDP)

set(CMAKE_CXX_STANDARD 17)
add_executable(main main.cpp)

find_package(Boost REQUIRED)
find_package(srs REQUIRED)
find_package(fmt REQUIRED)

target_link_libraries(main PRIVATE Boost::boost srs::srs fmt::fmt-header-only)
