#pragma once

#include "graphics/mesh.h"

#include <cstdint>

#include <vulkan/vulkan.h>

namespace vks {
struct VulkanDevice;
}

class MeshVk : public Mesh
{
public:
  MeshVk(vks::VulkanDevice* vulkanDevice,
         VkCommandPool commandPool,
         VkQueue queue,
         uint32_t i_nVertices,
         const float* i_Vertices,
         const float* i_Normals,
         const float* i_UVs,
         uint32_t i_nIndices,
         const uint32_t* i_Indices);

  BoundingBox getBoundingBox() override;

  VkBuffer getVertexBuffer() const { return m_vertexBuffer; }
  VkBuffer getIndexBuffer() const { return m_indexBuffer; }

  struct Vertex
  {
    glm::vec3 pos;
  };

private:
  vks::VulkanDevice* m_device = nullptr;
  VkCommandPool m_commandPool = VK_NULL_HANDLE;
  VkQueue m_queue = VK_NULL_HANDLE;

  VkBuffer m_vertexBuffer = VK_NULL_HANDLE, m_indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE, m_indexMemory = VK_NULL_HANDLE;
  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
};
