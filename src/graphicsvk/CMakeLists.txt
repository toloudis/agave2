add_library(graphicsvk STATIC 
"${CMAKE_CURRENT_SOURCE_DIR}/graphicsVk.h"
"${CMAKE_CURRENT_SOURCE_DIR}/graphicsVk.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/renderTargetVk.h"
"${CMAKE_CURRENT_SOURCE_DIR}/renderTargetVk.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/sceneRendererVk.h"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanBuffer.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanDebug.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanDebug.h"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanDevice.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanFrameBuffer.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanglTFModel.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanHeightmap.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanInitializers.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanModel.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanSwapChain.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanTexture.hpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanTools.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/VulkanTools.h"
)


target_include_directories(graphicsvk PUBLIC
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${Vulkan_INCLUDE_DIRS}
)
target_link_libraries(graphicsvk 
  graphics
  ${Vulkan_LIBRARY}
  ktx
)

