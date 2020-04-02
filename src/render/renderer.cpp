#include "renderer.h"

#include "VulkanTools.h"

#include "shaders/triangle.frag.h"
#include "shaders/triangle.vert.h"

#include <assert.h>

namespace renderlib {
VkShaderModule
loadShaderFromPtr(uint32_t* shaderCode, size_t size, VkDevice device)
{
  assert(size > 0);
  assert(shaderCode != nullptr);

  VkShaderModule shaderModule;
  VkShaderModuleCreateInfo moduleCreateInfo{};
  moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleCreateInfo.codeSize = size;
  moduleCreateInfo.pCode = (uint32_t*)shaderCode;

  VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

  return shaderModule;
}

};
