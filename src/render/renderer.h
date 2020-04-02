#pragma once

#include <vulkan/vulkan.h>

namespace renderlib {

VkShaderModule
loadShaderFromPtr(uint32_t* shaderCode, size_t size, VkDevice device);

};
