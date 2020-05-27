add_library(graphicsvk STATIC 
"${CMAKE_CURRENT_SOURCE_DIR}/graphicsVk.h"
"${CMAKE_CURRENT_SOURCE_DIR}/graphicsVk.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/renderTargetVk.h"
"${CMAKE_CURRENT_SOURCE_DIR}/sceneRendererVk.h"
)


target_include_directories(graphicsvk PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
#target_link_libraries(graphicsvk
#  graphics 
#)

