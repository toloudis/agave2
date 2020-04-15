#include "renderlib.h"

#include "VulkanTools.h"

#include <vector>

static VkInstance sInstance = nullptr;
static std::vector<VkPhysicalDevice> sPhysicalDevices;

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
  VkPhysicalDevice physicalDevice = sPhysicalDevices[which];

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  LOG("SELECTED GPU: %s\n", deviceProperties.deviceName);

  return physicalDevice;
}

void
renderlib::cleanup()
{
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
