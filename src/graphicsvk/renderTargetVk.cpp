#include "renderTargetVk.h"

RenderTargetVk::RenderTargetVk(vks::VulkanDevice* vulkanDevice, int width, int height, PixelFormat format)
{
  m_device = vulkanDevice;
  m_format = format;
  setSize(width, height);
}

void
RenderTargetVk::setSize(int width, int height)
{
  if (m_width != width || m_height != height) {
    // resize
    //   VkFormat
    //   VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(vulkanDevice->physicalDevice, &depthFormat);
    //   assert(validDepthFormat);
  }
  m_width = width;
  m_height = height;
}
