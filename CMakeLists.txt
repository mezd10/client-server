cmake_minimum_required(VERSION 3.15)
project(networkM)

set(CMAKE_CXX_STANDARD 14)

add_executable(networkM server.cpp)

find_package(PostgreSQL REQUIRED)
target_include_directories(networkM PRIVATE ${PostgreSQL_INCLUDE_DIRS})

# Add libraries to link your target againts. Again, PRIVATE is important for multi-target projects
target_link_libraries(networkM PRIVATE ${PostgreSQL_LIBRARIES} pthread)