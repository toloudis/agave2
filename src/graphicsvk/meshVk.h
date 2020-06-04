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
  VkCommandPool m_commandPool = nullptr;
  VkQueue m_queue = nullptr;

  VkBuffer m_vertexBuffer = nullptr, m_indexBuffer = nullptr;
  VkDeviceMemory m_vertexMemory = nullptr, m_indexMemory = nullptr;
  VkDescriptorPool m_descriptorPool = nullptr;
  VkDescriptorSetLayout m_descriptorSetLayout = nullptr;
};
