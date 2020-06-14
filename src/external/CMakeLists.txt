cmake_minimum_required(VERSION 2.8)

if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(external LANGUAGES C CXX)

# libktx

message(STATUS "Adding libktx")

set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/KTX-Software)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c
)
set(KTX_INCLUDE
    ${KTX_DIR}/include
    ${KTX_DIR}/lib
    ${KTX_DIR}/other_include
)

add_library(ktx ${KTX_SOURCES})
target_include_directories(ktx PUBLIC ${KTX_INCLUDE})
set_property(TARGET ktx PROPERTY FOLDER "external")

add_subdirectory(spdlog)

set(BUILD_TESTS OFF)
add_subdirectory(pugixml-1.10)
