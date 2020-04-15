#pragma once

#include <vulkan/vulkan.h>

#include <string>

class renderlib
{
public:
  // requiredExtensions are those required for OS-dependent windowing interaction.
  // for headless operation, defaults are valid.
  static int initialize(const char* appName = "",
                        uint32_t requiredExtensionCount = 0,
                        const char** requiredExtensionNames = nullptr);

  static void cleanup();

  static VkInstance instance();
  static VkPhysicalDevice selectPhysicalDevice(size_t which = 0);

  static VkShaderModule loadShaderFromPtr(uint32_t* shaderCode, size_t size, VkDevice device);
};