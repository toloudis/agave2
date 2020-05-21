#include "renderlib.h"

#include "shaders/triangle.frag.h"
#include "shaders/triangle.vert.h"

#include "VulkanDevice.hpp"
#include "VulkanTools.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <vector>

static VkInstance sInstance = nullptr;
static std::vector<VkPhysicalDevice> sPhysicalDevices;
static vks::VulkanDevice* sVulkanDevice = nullptr;
static std::vector<VkShaderModule> sShaderModules;

static VkDebugReportCallbackEXT debugReportCallback{};

#define DEBUG (!NDEBUG)

#define LOG(...) printf(__VA_ARGS__)

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugMessageCallback(VkDebugReportFlagsEXT flags,
                     VkDebugReportObjectTypeEXT objectType,
                     uint64_t object,
                     size_t location,
                     int32_t messageCode,
                     const char* pLayerPrefix,
                     const char* pMessage,
                     void* pUserData)
{
  LOG("[VALIDATION]: %s - %s\n", pLayerPrefix, pMessage);
  return VK_FALSE;
}

void
logPhysicalDevices()
{
  for (auto physicalDevice : sPhysicalDevices) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout << "Device : " << deviceProperties.deviceName << std::endl;
    std::cout << " Type: " << vks::tools::physicalDeviceTypeString(deviceProperties.deviceType) << std::endl;
    std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "." << ((deviceProperties.apiVersion >> 12) & 0x3ff)
              << "." << (deviceProperties.apiVersion & 0xfff) << std::endl;
  }
}

int
renderlib::initialize(const char* appName, uint32_t requiredExtensionCount, const char** requiredExtensionNames)
{
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = appName;
  appInfo.pEngineName = "AGAVE";
  appInfo.apiVersion = VK_API_VERSION_1_0;

  // Vulkan instance creation

  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = &appInfo;

  uint32_t layerCount = 0;

  const char* validationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
  layerCount = 1;

  std::vector<const char*> extensionNames;
  for (size_t i = 0; i < requiredExtensionCount; ++i) {
    extensionNames.push_back(requiredExtensionNames[i]);
  }

#if DEBUG
  // Check if layers are available
  uint32_t instanceLayerCount;
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
  std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

  bool layersAvailable = true;
  for (auto layerName : validationLayers) {
    bool layerAvailable = false;
    for (auto instanceLayer : instanceLayers) {
      if (strcmp(instanceLayer.layerName, layerName) == 0) {
        layerAvailable = true;
        break;
      }
    }
    if (!layerAvailable) {
      layersAvailable = false;
      break;
    }
  }

  if (layersAvailable) {
    instanceCreateInfo.ppEnabledLayerNames = validationLayers;
    instanceCreateInfo.enabledLayerCount = layerCount;

    extensionNames.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  }
#endif
  instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensionNames.size());
  instanceCreateInfo.ppEnabledExtensionNames = extensionNames.data();
  VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &sInstance));

#if DEBUG
  if (layersAvailable) {
    VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {};
    debugReportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugReportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    debugReportCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugMessageCallback;

    // We have to explicitly load this function.
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
      reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
        vkGetInstanceProcAddr(sInstance, "vkCreateDebugReportCallbackEXT"));
    assert(vkCreateDebugReportCallbackEXT);
    VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(sInstance, &debugReportCreateInfo, nullptr, &debugReportCallback));
  }
#endif

  // get all physical devices
  uint32_t deviceCount = 0;
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(sInstance, &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(sInstance, &deviceCount, physicalDevices.data()));
  sPhysicalDevices = physicalDevices;

  logPhysicalDevices();

  return 1;
}

VkPhysicalDevice
renderlib::selectPhysicalDevice(size_t which)
{
  assert(sPhysicalDevices.size() > 0);
  if (which >= sPhysicalDevices.size()) {
    which = 0;
  }
  VkPhysicalDevice physicalDevice = sPhysicalDevices[which];

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  LOG("SELECTED GPU: %s\n", deviceProperties.deviceName);

  return physicalDevice;
}

vks::VulkanDevice*
renderlib::createDevice(VkPhysicalDevice physicalDevice)
{
  sVulkanDevice = new vks::VulkanDevice(physicalDevice);
  VkPhysicalDeviceFeatures enabledFeatures{};
  std::vector<const char*> enabledDeviceExtensions;
  VkResult res = sVulkanDevice->createLogicalDevice(enabledFeatures, enabledDeviceExtensions, nullptr, false);
  if (res != VK_SUCCESS) {
    vks::tools::exitFatal("Could not create Vulkan device: \n" + vks::tools::errorString(res), res);
    return nullptr;
  }

  return sVulkanDevice;
}

void
renderlib::cleanup()
{

  for (auto shadermodule : sShaderModules) {
    vkDestroyShaderModule(sVulkanDevice->logicalDevice, shadermodule, nullptr);
  }
  delete sVulkanDevice;

#if DEBUG
  if (debugReportCallback) {
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback =
      reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
        vkGetInstanceProcAddr(sInstance, "vkDestroyDebugReportCallbackEXT"));
    assert(vkDestroyDebugReportCallback);
    vkDestroyDebugReportCallback(sInstance, debugReportCallback, nullptr);
  }
#endif
  vkDestroyInstance(sInstance, nullptr);
}

VkInstance
renderlib::instance()
{
  return sInstance;
}

VkShaderModule
renderlib::loadShaderFromPtr(uint32_t* shaderCode, size_t size, VkDevice device)
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

VkResult
renderlib::createGraphicsPipeline(VkDevice device,
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
    vks::initializers::vertexInputBindingDescription(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
  };

  // Attribute descriptions
  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
    vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0),                 // Position
    vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3), // Color
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

  shaderStages[0].module =
    renderlib::loadShaderFromPtr((uint32_t*)shaders_triangle_vert, sizeof(shaders_triangle_vert), device);
  shaderStages[1].module =
    renderlib::loadShaderFromPtr((uint32_t*)shaders_triangle_frag, sizeof(shaders_triangle_frag), device);

  sShaderModules = { shaderStages[0].module, shaderStages[1].module };
  return vkCreateGraphicsPipelines(device, *pipelineCache, 1, &pipelineCreateInfo, nullptr, pipeline);
}
