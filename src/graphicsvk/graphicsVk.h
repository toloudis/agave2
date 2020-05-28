
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

private:
  VkPhysicalDevice selectPhysicalDevice(size_t which);
  vks::VulkanDevice* createDevice(VkPhysicalDevice physicalDevice);
};
