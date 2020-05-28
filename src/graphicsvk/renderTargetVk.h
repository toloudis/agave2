#pragma once

#include "graphics/graphics.h"
#include "graphics/renderTarget.h"

namespace vks {
struct VulkanDevice;
}

// abstract either a buffer of image pixels or a swapchain tied to a window
class RenderTargetVk : public RenderTarget
{
public:
  RenderTargetVk(vks::VulkanDevice* vulkanDevice, int width, int height, PixelFormat format);

  // flush all rendering commands and swapbuffers
  void swap() override {}
  // block till rendering is done and return pixels data as a pointer to system memory
  void* getPixels(/* optional rectangle ? */) override { return nullptr; }
  void setSize(int width, int height) override;
  int getWidth() override { return m_width; }
  int getHeight() override { return m_height; }

private:
  vks::VulkanDevice* m_device = nullptr;
  int m_width = 0;
  int m_height = 0;
  PixelFormat m_format = PixelFormat::RGBA8U;
};