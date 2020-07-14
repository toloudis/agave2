add_library(graphics STATIC 
"${CMAKE_CURRENT_SOURCE_DIR}/boundingBox.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/boundingBox.h"
"${CMAKE_CURRENT_SOURCE_DIR}/camera.h"
"${CMAKE_CURRENT_SOURCE_DIR}/defines.h"
"${CMAKE_CURRENT_SOURCE_DIR}/gradientData.h"
"${CMAKE_CURRENT_SOURCE_DIR}/graphics.h"
"${CMAKE_CURRENT_SOURCE_DIR}/histogram.h"
"${CMAKE_CURRENT_SOURCE_DIR}/histogram.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/imageXYZC.h"
"${CMAKE_CURRENT_SOURCE_DIR}/imageXYZC.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/mesh.h"
"${CMAKE_CURRENT_SOURCE_DIR}/renderTarget.h"
"${CMAKE_CURRENT_SOURCE_DIR}/scene.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/scene.h"
"${CMAKE_CURRENT_SOURCE_DIR}/sceneObject.h"
"${CMAKE_CURRENT_SOURCE_DIR}/sceneObject.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/sceneRenderer.h"
"${CMAKE_CURRENT_SOURCE_DIR}/timeline.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/timeline.h"
"${CMAKE_CURRENT_SOURCE_DIR}/volume.h"
"${CMAKE_CURRENT_SOURCE_DIR}/volumeDimensions.h"
"${CMAKE_CURRENT_SOURCE_DIR}/volumeDimensions.cpp"
)

# set_target_properties(graphics PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(graphics PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
#target_link_libraries(graphics 
#)

