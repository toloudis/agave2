#include "graphicsVk.h"

#include "meshVk.h"
#include "renderTargetVk.h"
#include "sceneRendererVk.h"

#include "VulkanDevice.hpp"
#include "VulkanFrameBuffer.hpp"
#include "VulkanTools.h"
#include <vulkan/vulkan.h>

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
GraphicsVk::logPhysicalDevices()
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

GraphicsVk::GraphicsVk() {}

GraphicsVk::~GraphicsVk() {}

bool
GraphicsVk::init()
{
  // init vulkan,
  // choose a default graphics device for top level functions,
  // prepare other devices for other computation tasks?

  uint32_t requiredExtensionCount = 0;
  const char** requiredExtensionNames = nullptr;
  const char* appName = "APP NAME";

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = appName;
  appInfo.pEngineName = "AGAVE2";
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

  // select a default device
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  vks::VulkanDevice* vulkanDevice = nullptr;
  physicalDevice = selectPhysicalDevice(1);
  vulkanDevice = createDevice(physicalDevice);
  device = vulkanDevice->logicalDevice;

  return true;
}

VkPhysicalDevice
GraphicsVk::selectPhysicalDevice(size_t which)
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
GraphicsVk::createDevice(VkPhysicalDevice physicalDevice)
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

bool
GraphicsVk::cleanup()
{
  // this must be called LATE.  All objects created from here must already be destroyed.
  // destroy everything created after the device
  // and then destroy the device and the instance

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

  return true;
}

SceneRenderer*
GraphicsVk::createDefaultRenderer()
{
  return new SceneRendererVk(sVulkanDevice);
}

SceneRenderer*
GraphicsVk::createNormalsRenderer()
{
  return nullptr;
}

RenderTarget*
GraphicsVk::createWindowRenderTarget()
{
  return nullptr;
}

RenderTarget*
GraphicsVk::createImageRenderTarget(int width, int height, PixelFormat format)
{
  RenderTargetVk* rt = new RenderTargetVk(sVulkanDevice, width, height, format);
  return rt;
}

Mesh*
GraphicsVk::createMesh(uint32_t i_nVertices,
                       const float* i_Vertices,
                       const float* i_Normals,
                       const float* i_UVs,
                       uint32_t i_nIndices,
                       const uint32_t* i_Indices)
{
  MeshVk* mesh = new MeshVk(sVulkanDevice, i_nVertices, i_Vertices, i_Normals, i_UVs, i_nIndices, i_Indices);
  return mesh;
}
