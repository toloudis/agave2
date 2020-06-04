#include "renderTargetVk.h"

#include "VulkanFrameBuffer.hpp"

// TODO format!!!!
static const int BYTES_PER_PIXEL = 4;

RenderTargetVk::RenderTargetVk(vks::VulkanDevice* vulkanDevice,
                               VkQueue queue,
                               int width,
                               int height,
                               PixelFormat format)
{
  m_device = vulkanDevice;
  m_queue = queue;
  m_format = format;
  setSize(width, height);
}

RenderTargetVk::~RenderTargetVk()
{
  delete[] m_image;
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

  delete[] m_image;

  m_image = new uint8_t[m_width * m_height * BYTES_PER_PIXEL];

  /*
          Create framebuffer attachments
  */
  VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
  VkFormat depthFormat;
  vks::tools::getSupportedDepthFormat(m_device->physicalDevice, &depthFormat);

  m_vulkanFramebuffer = new vks::Framebuffer(m_device);
  m_vulkanFramebuffer->width = width;
  m_vulkanFramebuffer->height = height;
  // Color attachment
  vks::AttachmentCreateInfo colorAttachmentCreateInfo;
  colorAttachmentCreateInfo.width = width;
  colorAttachmentCreateInfo.height = height;
  colorAttachmentCreateInfo.layerCount = 1;
  colorAttachmentCreateInfo.format = colorFormat;
  colorAttachmentCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  m_vulkanFramebuffer->addAttachment(colorAttachmentCreateInfo);

  vks::AttachmentCreateInfo depthAttachmentCreateInfo;
  depthAttachmentCreateInfo.width = width;
  depthAttachmentCreateInfo.height = height;
  depthAttachmentCreateInfo.layerCount = 1;
  depthAttachmentCreateInfo.format = depthFormat;
  depthAttachmentCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  m_vulkanFramebuffer->addAttachment(depthAttachmentCreateInfo);

  /*
        Create renderpass
*/
  VK_CHECK_RESULT(m_vulkanFramebuffer->createRenderPass());
}

// flush all rendering commands and swapbuffers
void
RenderTargetVk::swap()
{
  vkDeviceWaitIdle(m_device->logicalDevice);
}

// block till rendering is done and return pixels data as a pointer to system memory
void* RenderTargetVk::getPixels(/* optional rectangle ? */)
{
  VkDevice device = m_device->logicalDevice;
  /*
        Copy framebuffer image to host visible image
*/
  const char* imagedata;
  {
    // Create the linear tiled destination image to copy to and to read the
    // memory from
    VkImageCreateInfo imgCreateInfo(vks::initializers::imageCreateInfo());
    imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imgCreateInfo.extent.width = m_width;
    imgCreateInfo.extent.height = m_height;
    imgCreateInfo.extent.depth = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // Create the image
    VkImage dstImage;
    VK_CHECK_RESULT(vkCreateImage(device, &imgCreateInfo, nullptr, &dstImage));
    // Create memory to back up the image
    VkMemoryRequirements memRequirements;
    VkMemoryAllocateInfo memAllocInfo(vks::initializers::memoryAllocateInfo());
    VkDeviceMemory dstImageMemory;
    vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
    memAllocInfo.allocationSize = memRequirements.size;
    // Memory must be host visible to copy from
    memAllocInfo.memoryTypeIndex = m_device->getMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &dstImageMemory));
    VK_CHECK_RESULT(vkBindImageMemory(device, dstImage, dstImageMemory, 0));

    // Do the actual blit from the offscreen image to our host visible
    // destination image
    VkCommandBufferAllocateInfo cmdBufAllocateInfo =
      vks::initializers::commandBufferAllocateInfo(m_device->commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
    VkCommandBuffer copyCmd;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd));
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
    VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo));

    // Transition destination image to transfer destination layout
    vks::tools::insertImageMemoryBarrier(copyCmd,
                                         dstImage,
                                         0,
                                         VK_ACCESS_TRANSFER_WRITE_BIT,
                                         VK_IMAGE_LAYOUT_UNDEFINED,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    // colorAttachment.image is already in
    // VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and does not need to be
    // transitioned

    VkImageCopy imageCopyRegion{};
    imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width = m_width;
    imageCopyRegion.extent.height = m_height;
    imageCopyRegion.extent.depth = 1;

    vkCmdCopyImage(copyCmd,
                   m_vulkanFramebuffer->attachments[0].image,
                   // colorAttachment.image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   dstImage,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &imageCopyRegion);

    // Transition destination image to general layout, which is the required
    // layout for mapping the image memory later on
    vks::tools::insertImageMemoryBarrier(copyCmd,
                                         dstImage,
                                         VK_ACCESS_TRANSFER_WRITE_BIT,
                                         VK_ACCESS_MEMORY_READ_BIT,
                                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                         VK_IMAGE_LAYOUT_GENERAL,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                                         VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

    // VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd));
    m_device->flushCommandBuffer(copyCmd, m_queue, false);

    // Get layout of the image (including row pitch)
    VkImageSubresource subResource{};
    subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    VkSubresourceLayout subResourceLayout;

    vkGetImageSubresourceLayout(device, dstImage, &subResource, &subResourceLayout);

    // Map image memory so we can start copying from it
    vkMapMemory(device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&imagedata);
    imagedata += subResourceLayout.offset;

    /*
    copy into a system memory buffer big enough to hold the image.

            Save host visible framebuffer image to disk (ppm format)
    */
    // If source is BGR (destination is always RGB) and we can't use blit
    // (which does automatic conversion), we'll have to manually swizzle color
    // components Check if source is BGR and needs swizzle
    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
    const bool colorSwizzle =
      (std::find(formatsBGR.begin(), formatsBGR.end(), VK_FORMAT_R8G8B8A8_UNORM) != formatsBGR.end());

    // TODO: handle colorSwizzle
    uint8_t* dst = m_image;
    for (int32_t y = 0; y < m_height; y++) {
      memcpy(dst, imagedata, m_width * BYTES_PER_PIXEL);
      imagedata += subResourceLayout.rowPitch;
      dst += m_width * BYTES_PER_PIXEL;
    }

    // Clean up resources
    vkUnmapMemory(device, dstImageMemory);
    vkFreeMemory(device, dstImageMemory, nullptr);
    vkDestroyImage(device, dstImage, nullptr);
  }

  vkQueueWaitIdle(m_queue);
  return m_image;
}
