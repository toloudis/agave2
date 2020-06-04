
#pragma once

#include "graphics/graphics.h"

namespace vks {
struct VulkanDevice;
}

#include <vulkan/vulkan.h>

class GraphicsVk : public Graphics
{
public:
  GraphicsVk();
  ~GraphicsVk();

  bool init() override;
  bool cleanup() override;

  SceneRenderer* createDefaultRenderer() override;
  SceneRenderer* createNormalsRenderer() override;

  RenderTarget* createWindowRenderTarget() override;
  RenderTarget* createImageRenderTarget(int width, int height, PixelFormat format = PixelFormat::RGBA8U) override;

  Mesh* createMesh(uint32_t i_nVertices,
                   const float* i_Vertices,
                   const float* i_Normals,
                   const float* i_UVs,
                   uint32_t i_nIndices,
                   const uint32_t* i_Indices) override;

private:
  VkInstance createInstance();

  VkPhysicalDevice selectPhysicalDevice(size_t which = 0);
  vks::VulkanDevice* createDevice(VkPhysicalDevice physicalDevice);
  void logPhysicalDevices();
};
