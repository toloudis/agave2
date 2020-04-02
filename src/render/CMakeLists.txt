add_subdirectory(shaders)

spirv_shaders(SHADER_HEADERS
  shaders/triangle.vert
  shaders/triangle.frag
)

add_library(render STATIC 
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/renderer.h"
  ${SHADER_HEADERS}
)


target_include_directories(render PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${Vulkan_INCLUDE_DIRS}
)
target_link_libraries(render 
  ${Vulkan_LIBRARY}
  base
)

