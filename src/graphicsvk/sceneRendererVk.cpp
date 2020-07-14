#include "sceneRendererVk.h"

#include "VulkanDevice.hpp"
#include "VulkanFrameBuffer.hpp"
#include "graphicsVk.h"
#include "meshVk.h"
#include "renderTargetVk.h"

#include "shaders/triangle.frag.h"
#include "shaders/triangle.vert.h"

#include "graphics/scene.h"
#include "graphics/sceneObject.h"

#include <glm/glm.hpp>

SceneRendererVk::SceneRendererVk(vks::VulkanDevice* vulkanDevice)
{
  m_device = vulkanDevice;

  uint32_t queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphics;
  // Get a graphics queue
  vkGetDeviceQueue(vulkanDevice->logicalDevice, queueFamilyIndex, 0, &m_queue);

  // Command pool
  m_commandPool = vulkanDevice->createCommandPool(queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  m_triangleVS = GraphicsVk::loadShaderFromPtr(
    (uint32_t*)shaders_triangle_vert, sizeof(shaders_triangle_vert), vulkanDevice->logicalDevice);
  m_triangleFS = GraphicsVk::loadShaderFromPtr(
    (uint32_t*)shaders_triangle_frag, sizeof(shaders_triangle_frag), vulkanDevice->logicalDevice);
}

SceneRendererVk::~SceneRendererVk()
{
  vkDestroyShaderModule(m_device->logicalDevice, m_triangleVS, nullptr);
  vkDestroyShaderModule(m_device->logicalDevice, m_triangleFS, nullptr);
}

void
SceneRendererVk::render(RenderTarget* target, const Camera& camera, const Scene& scene, float simulationTime)
{
  RenderTargetVk* renderTarget = dynamic_cast<RenderTargetVk*>(target);
  if (!renderTarget) {
    std::cout << "BAD RENDER TARGET TYPE!";
    return;
  }

  VkPipeline pipeline;
  VkPipelineCache pipelineCache;
  VkPipelineLayout pipelineLayout;
  VK_CHECK_RESULT(createGraphicsPipeline(
    m_device->logicalDevice, renderTarget->fb()->renderPass, &pipeline, &pipelineCache, &pipelineLayout));

  /*
          Command buffer creation
  */
  VkCommandBuffer commandBuffer = m_device->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, m_commandPool, false);

  {
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

    VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));

    // one per attachment
    VkClearValue clearValues[2];
    clearValues[0].color = { { 1.0f, 0.0f, 0.0f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderArea.extent.width = target->getWidth();
    renderPassBeginInfo.renderArea.extent.height = target->getHeight();
    // one per attachment
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

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Render scene
    //
    for (SceneObject* obj : scene.m_objects) {
      Mesh* m = obj->getMesh();
      MeshVk* mvk = dynamic_cast<MeshVk*>(m);
      if (mvk) {
        VkBuffer vb = mvk->getVertexBuffer();
        VkBuffer ib = mvk->getIndexBuffer();

        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vb, offsets);
        vkCmdBindIndexBuffer(commandBuffer, ib, 0, VK_INDEX_TYPE_UINT32);

        // 3 instances of the same mesh!  bind vtx and index buf only once.
        std::vector<glm::vec3> pos = {
          glm::vec3(-1.5f, 0.0f, -4.0f),
          glm::vec3(0.0f, 0.0f, -2.5f),
          glm::vec3(1.5f, 0.0f, -4.0f),
        };

        for (auto v : pos) {
          glm::mat4 mvpMatrix =
            glm::perspective(
              glm::radians(60.0f), (float)target->getWidth() / (float)target->getHeight(), 0.1f, 256.0f) *
            glm::translate(glm::mat4(1.0f), v);
          vkCmdPushConstants(
            commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mvpMatrix), &mvpMatrix);
          vkCmdDrawIndexed(commandBuffer, 3, 1, 0, 0, 0);
        }

      } else {
        std::cout << "Not a vulkan mesh??";
      }
    }

    vkCmdEndRenderPass(commandBuffer);

    m_device->flushCommandBuffer(commandBuffer, m_queue, false);
    // VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
    // submitWork(commandBuffer, queue);

    // TODO necessary here?  swapbuffers should handle this
    vkDeviceWaitIdle(m_device->logicalDevice);
  }
}

VkResult
SceneRendererVk::createGraphicsPipeline(VkDevice device,
                                        VkRenderPass renderPass,
                                        VkPipeline* pipeline,
                                        VkPipelineCache* pipelineCache,
                                        VkPipelineLayout* pipelineLayout)
{
  // TODO from caller, must be cleaned up later.
  VkDescriptorSetLayout descriptorSetLayout;

  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
  VkDescriptorSetLayoutCreateInfo descriptorLayout =
    vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(nullptr, 0);

  // MVP(modelviewprojection) via push constant block
  VkPushConstantRange pushConstantRange =
    vks::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4), 0);
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, pipelineLayout));

  VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, pipelineCache));

  // Create pipeline
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
    vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);

  VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(
    VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

  VkPipelineColorBlendAttachmentState blendAttachmentState =
    vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

  VkPipelineColorBlendStateCreateInfo colorBlendState =
    vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

  VkPipelineDepthStencilStateCreateInfo depthStencilState =
    vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

  VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1);

  VkPipelineMultisampleStateCreateInfo multisampleState =
    vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
  VkPipelineDynamicStateCreateInfo dynamicState =
    vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

  VkGraphicsPipelineCreateInfo pipelineCreateInfo = vks::initializers::pipelineCreateInfo(*pipelineLayout, renderPass);

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();

  // Vertex bindings an attributes
  // Binding description
  std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
    vks::initializers::vertexInputBindingDescription(0, sizeof(MeshVk::Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
  };

  // Attribute descriptions
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
    vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0), // Position
    //    vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3), //
    //    Color
  };

  VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
  vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
  vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
  vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
  vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

  pipelineCreateInfo.pVertexInputState = &vertexInputState;

  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].pName = "main";
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].pName = "main";

  shaderStages[0].module = m_triangleVS;
  shaderStages[1].module = m_triangleFS;

  return vkCreateGraphicsPipelines(device, *pipelineCache, 1, &pipelineCreateInfo, nullptr, pipeline);
}
