#include "meshVk.h"

#include "VulkanDevice.hpp"

MeshVk::MeshVk(vks::VulkanDevice* vulkanDevice,
               uint32_t i_nVertices,
               const float* i_Vertices,
               const float* i_Normals,
               const float* i_UVs,
               uint32_t i_nIndices,
               const uint32_t* i_Indices)
{
  m_device = vulkanDevice;

  const VkDeviceSize vertexBufferSize = i_nVertices * sizeof(float) * 3;
  const VkDeviceSize indexBufferSize = i_nIndices * sizeof(uint32_t);

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;
#if 0

  VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandPool);
  VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
  // Copy input data to VRAM using a staging buffer
  {
    // Vertices
    VK_CHECK_RESULT(
      vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 vertexBufferSize,
                                 &stagingBuffer,
                                 &stagingMemory,
                                 i_Vertices));

    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               vertexBufferSize,
                                               &vertexBuffer,
                                               &vertexMemory));

    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
    VkBufferCopy copyRegion = {};
    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffer, vertexBuffer, 1, &copyRegion);
    VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

    submitWork(copyCmd, queue);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    // Indices
    VK_CHECK_RESULT(
      vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 indexBufferSize,
                                 &stagingBuffer,
                                 &stagingMemory,
                                 i_Indices));

    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               indexBufferSize,
                                               &indexBuffer,
                                               &indexMemory));

    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffer, indexBuffer, 1, &copyRegion);
    VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

    submitWork(copyCmd, queue);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
  }
#endif
}

BoundingBox
MeshVk::getBoundingBox()
{
  return BoundingBox();
}
