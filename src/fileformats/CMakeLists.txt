add_library(fileformats STATIC 
"${CMAKE_CURRENT_SOURCE_DIR}/fileReader.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/fileReader.h"	
"${CMAKE_CURRENT_SOURCE_DIR}/fileReaderCzi.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/fileReaderCzi.h"	
"${CMAKE_CURRENT_SOURCE_DIR}/fileReaderTIFF.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/fileReaderTIFF.h"	
)

target_include_directories(fileformats PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)


# libCZI dependency for renderlib
add_compile_definitions(_STATICLIBBUILD)
# I prefer target_compile_definitions but the target has to exist first
add_subdirectory(libCZI/Src/JxrDecode)
add_subdirectory(libCZI/Src/libCZI)
find_library(libCZI REQUIRED PATHS ${CMAKE_SOURCE_DIR}/libCZI)
message(STATUS "libCZI: ${libCZI}")
set_target_properties(libCZI PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
# end libczi dependency

# libtiff dependency for renderlib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(libtiff)
find_library(tiff REQUIRED PATHS ${CMAKE_BINARY_DIR}/renderlib/libtiff/libtiff)
message(STATUS "libtiff: ${CMAKE_BINARY_DIR}/renderlib/libtiff/libtiff")
message(STATUS "libtiff: ${tiff}")
# end libtiff dependency

target_link_libraries(fileformats
	${CMAKE_DL_LIBS}
	tiff
	libCZIStatic
	JxrDecodeStatic # libCZI depends on it
)


