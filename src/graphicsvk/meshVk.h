#pragma once

#include "graphics/mesh.h"

#include <cstdint>

namespace vks {
struct VulkanDevice;
}

class MeshVk : public Mesh
{
public:
  MeshVk(vks::VulkanDevice* vulkanDevice,
         uint32_t i_nVertices,
         const float* i_Vertices,
         const float* i_Normals,
         const float* i_UVs,
         uint32_t i_nIndices,
         const uint32_t* i_Indices);

  BoundingBox getBoundingBox() override;

private:
  vks::VulkanDevice* m_device;
};
