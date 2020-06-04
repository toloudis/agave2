#include "sceneRendererVk.h"

#include "VulkanDevice.hpp"
#include "VulkanFrameBuffer.hpp"
#include "renderTargetVk.h"

SceneRendererVk::SceneRendererVk(vks::VulkanDevice* vulkanDevice)
{
  m_device = vulkanDevice;

  uint32_t queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
  // Get a graphics queue
  vkGetDeviceQueue(vulkanDevice->logicalDevice, queueFamilyIndex, 0, &m_queue);

  // Command pool
  m_commandPool = vulkanDevice->createCommandPool(queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
}

void
SceneRendererVk::render(RenderTarget* target, const Camera& camera, const Scene& scene, float simulationTime)
{
  RenderTargetVk* renderTarget = dynamic_cast<RenderTargetVk*>(target);
  if (!renderTarget) {
    std::cout << "BAD RENDER TARGET TYPE!";
    return;
  }
  /*
          Command buffer creation
  */
  VkCommandBuffer commandBuffer = m_device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_commandPool, false);

  {
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.2f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderArea.extent.width = target->getWidth();
    renderPassBeginInfo.renderArea.extent.height = target->getHeight();
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.renderPass = renderTarget->fb()->renderPass;
    renderPassBeginInfo.framebuffer = renderTarget->fb()->framebuffer;

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.height = (float)target->getHeight();
    viewport.width = (float)target->getWidth();
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // Update dynamic scissor state
    VkRect2D scissor = {};
    scissor.extent.width = target->getWidth();
    scissor.extent.height = target->getHeight();
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
#if 0
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Render scene
    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    std::vector<glm::vec3> pos = {
      glm::vec3(-1.5f, 0.0f, -4.0f),
      glm::vec3(0.0f, 0.0f, -2.5f),
      glm::vec3(1.5f, 0.0f, -4.0f),
    };

    for (auto v : pos) {
      glm::mat4 mvpMatrix = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f) *
                            glm::translate(glm::mat4(1.0f), v);
      vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvpMatrix), &mvpMatrix);
      vkCmdDrawIndexed(commandBuffer, 3, 1, 0, 0, 0);
    }
#endif
    vkCmdEndRenderPass(commandBuffer);

    m_device->flushCommandBuffer(commandBuffer, m_queue, false);
    // VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    // submitWork(commandBuffer, queue);

    // TODO necessary here?  swapbuffers should handle this
    vkDeviceWaitIdle(m_device->logicalDevice);
  }
}
