add_library(graphics STATIC 
"${CMAKE_CURRENT_SOURCE_DIR}/graphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/renderTarget.h"
"${CMAKE_CURRENT_SOURCE_DIR}/sceneRenderer.h"
)

set_target_properties(graphics PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(graphics PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
#target_link_libraries(graphics 
#)

