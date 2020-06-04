#include "meshVk.h"

#include "VulkanDevice.hpp"

MeshVk::MeshVk(vks::VulkanDevice* vulkanDevice,
               VkCommandPool commandpool,
               VkQueue queue,
               uint32_t i_nVertices,
               const float* i_Vertices,
               const float* i_Normals,
               const float* i_UVs,
               uint32_t i_nIndices,
               const uint32_t* i_Indices)
{
  m_device = vulkanDevice;
  m_commandPool = commandpool;
  m_queue = queue;

  const VkDeviceSize vertexBufferSize = i_nVertices * sizeof(float) * 3;
  const VkDeviceSize indexBufferSize = i_nIndices * sizeof(uint32_t);

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandpool);
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
                                 (void*)i_Vertices));

    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               vertexBufferSize,
                                               &m_vertexBuffer,
                                               &m_vertexMemory));

    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
    VkBufferCopy copyRegion = {};
    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffer, m_vertexBuffer, 1, &copyRegion);
    // VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));

    vulkanDevice->flushCommandBuffer(copyCmd, queue, false);

    vkDestroyBuffer(vulkanDevice->logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(vulkanDevice->logicalDevice, stagingMemory, nullptr);

    // Indices
    VK_CHECK_RESULT(
      vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 indexBufferSize,
                                 &stagingBuffer,
                                 &stagingMemory,
                                 (void*)i_Indices));

    VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                               indexBufferSize,
                                               &m_indexBuffer,
                                               &m_indexMemory));

    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));
    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingBuffer, m_indexBuffer, 1, &copyRegion);
    // VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));
    vulkanDevice->flushCommandBuffer(copyCmd, queue, false);

    // submitWork(copyCmd, queue);

    vkDestroyBuffer(vulkanDevice->logicalDevice, stagingBuffer, nullptr);
    vkFreeMemory(vulkanDevice->logicalDevice, stagingMemory, nullptr);
  }

  std::cout << "mesh created on gpu" << std::endl;
}

BoundingBox
MeshVk::getBoundingBox()
{
  return BoundingBox();
}
