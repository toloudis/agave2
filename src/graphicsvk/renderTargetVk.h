#pragma once

#include "graphics/graphics.h"
#include "graphics/renderTarget.h"

#include <vulkan/vulkan.h>

namespace vks {
struct VulkanDevice;
struct Framebuffer;
}

// abstract either a buffer of image pixels or a swapchain tied to a window
class RenderTargetVk : public RenderTarget
{
public:
  RenderTargetVk(vks::VulkanDevice* vulkanDevice, VkQueue queue, int width, int height, PixelFormat format);
  ~RenderTargetVk();

  // flush all rendering commands and swapbuffers
  void swap() override;
  // block till rendering is done and return pixels data as a pointer to system memory
  void* getPixels(/* optional rectangle ? */) override;
  void setSize(int width, int height) override;
  int getWidth() override { return m_width; }
  int getHeight() override { return m_height; }

  vks::Framebuffer* fb() const { return m_vulkanFramebuffer; }

private:
  vks::VulkanDevice* m_device = nullptr;
  VkQueue m_queue = nullptr;
  int m_width = 0;
  int m_height = 0;
  PixelFormat m_format = PixelFormat::RGBA8U;

  vks::Framebuffer* m_vulkanFramebuffer = nullptr;

  // byte array to hold the image.
  uint8_t* m_image = nullptr;
};