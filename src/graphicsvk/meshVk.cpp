#include "meshVk.h"

MeshVk::MeshVk(vks::VulkanDevice* vulkanDevice,
               uint32_t i_nVertices,
               const float* i_Vertices,
               const float* i_Normals,
               const float* i_UVs,
               uint32_t i_nIndices,
               const uint32_t* i_Indices)
{
  m_device = vulkanDevice;
}

BoundingBox
MeshVk::getBoundingBox()
{
  return BoundingBox();
}
