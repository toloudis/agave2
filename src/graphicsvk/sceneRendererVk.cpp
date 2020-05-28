#include "sceneRendererVk.h"

#include "VulkanDevice.hpp"

SceneRendererVk::SceneRendererVk(vks::VulkanDevice* vulkanDevice)
{
  m_device = vulkanDevice;

  uint32_t queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
  // Get a graphics queue
  VkQueue queue;
  vkGetDeviceQueue(vulkanDevice->logicalDevice, queueFamilyIndex, 0, &queue);

  // Command pool
  VkCommandPool commandPool =
    vulkanDevice->createCommandPool(queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}

void
SceneRendererVk::render(RenderTarget* target, const Camera& camera, const Scene& scene, float simulationTime)
{}
