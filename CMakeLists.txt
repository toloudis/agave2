cmake_minimum_required(VERSION 3.17)
project(
    agave2
    VERSION 1.0.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 14)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory(src)

