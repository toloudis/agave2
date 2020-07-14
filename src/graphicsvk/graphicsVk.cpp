#include "graphicsVk.h"

#include "meshVk.h"
#include "renderTargetVk.h"
#include "sceneRendererVk.h"

#include "VulkanDebug.h"
#include "VulkanDevice.hpp"
#include "VulkanFrameBuffer.hpp"
#include "VulkanTools.h"
#include <vulkan/vulkan.h>

static bool g_validation = true;
static std::vector<const char*> enabledInstanceExtensions;

static VkInstance sInstance = nullptr;
static std::vector<VkPhysicalDevice> sPhysicalDevices;
static vks::VulkanDevice* sVulkanDevice = nullptr;
// associated with device.  device, commandpool, and queue could potentially be grouped together
static VkCommandPool sCommandPool = nullptr;
static VkQueue sQueue = nullptr;

static std::vector<VkShaderModule> sShaderModules;

#define DEBUG (!NDEBUG)

#define LOG(...) printf(__VA_ARGS__)

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

  sInstance = createInstance();

  // If requested, we enable the default validation layers for debugging
  if (g_validation) {
    // The report flags determine what type of messages for the layers will be displayed
    // For validating (debugging) an appplication the error and warning bits should suffice
    VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    // Additional flags include performance info, loader and layer debug messages, etc.
    vks::debug::setupDebugging(sInstance, debugReportFlags, VK_NULL_HANDLE);
  }

  // get all physical devices
  uint32_t deviceCount = 0;
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(sInstance, &deviceCount, nullptr));
  if (deviceCount == 0) {
    return false;
  }
  std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(sInstance, &deviceCount, physicalDevices.data()));
  sPhysicalDevices = physicalDevices;

  logPhysicalDevices();

  // select a default device
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  vks::VulkanDevice* vulkanDevice = nullptr;
  physicalDevice = selectPhysicalDevice();
  vulkanDevice = createDevice(physicalDevice);
  device = vulkanDevice->logicalDevice;

  if (vulkanDevice->enableDebugMarkers) {
    vks::debugmarker::setup(device);
  }

  return true;
}

VkInstance
GraphicsVk::createInstance()
{
  uint32_t requiredExtensionCount = 0;
  // TODO VK_KHR_SURFACE_EXTENSION_NAME
  const char** requiredExtensionNames = nullptr;
  const char* appName = "APP NAME";

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = appName;
  appInfo.pEngineName = "AGAVE2";
  // TODO: 1.1?  1.2?  check availability vkEnumerateInstanceVersion
  appInfo.apiVersion = VK_API_VERSION_1_0;

  // Vulkan instance creation

  std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

  /***
    // Enable surface extensions depending on os
  #if defined(_WIN32)
    instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
  #elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    instanceExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
  #elif defined(_DIRECT2DISPLAY)
    instanceExtensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
  #elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
  #elif defined(VK_USE_PLATFORM_XCB_KHR)
    instanceExtensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
  #elif defined(VK_USE_PLATFORM_IOS_MVK)
    instanceExtensions.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
  #elif defined(VK_USE_PLATFORM_MACOS_MVK)
    instanceExtensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
  #endif
  ****/

  enabledInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

  if (enabledInstanceExtensions.size() > 0) {
    for (auto enabledExtension : enabledInstanceExtensions) {
      instanceExtensions.push_back(enabledExtension);
    }
  }

  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pNext = NULL;
  instanceCreateInfo.pApplicationInfo = &appInfo;
  if (instanceExtensions.size() > 0) {
    if (g_validation) {
      instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
  }
  if (g_validation) {
    // The VK_LAYER_KHRONOS_validation contains all current validation functionality.
    // Note that on Android this layer requires at least NDK r20
    const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
    // Check if this layer is available at instance level
    uint32_t instanceLayerCount;
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
    std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
    bool validationLayerPresent = false;
    for (VkLayerProperties layer : instanceLayerProperties) {
      if (strcmp(layer.layerName, validationLayerName) == 0) {
        validationLayerPresent = true;
        break;
      }
    }
    if (validationLayerPresent) {
      instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
      instanceCreateInfo.enabledLayerCount = 1;
    } else {
      std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
    }
  }

  VkInstance instance = nullptr;
  VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

  return instance;
}

VkPhysicalDevice
GraphicsVk::selectPhysicalDevice(size_t which)
{
  assert(sPhysicalDevices.size() > 0);

  // pick first discrete gpu or failing that, pick first gpu.
  for (size_t i = 0; i < sPhysicalDevices.size(); ++i) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(sPhysicalDevices[i], &deviceProperties);
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      which = i;
      break;
    }
  }

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

  VkCommandPool graphicsCommandPool = sVulkanDevice->createCommandPool(sVulkanDevice->queueFamilyIndices.graphics,
                                                                       VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
  VkQueue graphicsQueue;
  vkGetDeviceQueue(sVulkanDevice->logicalDevice, sVulkanDevice->queueFamilyIndices.graphics, 0, &graphicsQueue);
  sCommandPool = graphicsCommandPool;
  sQueue = graphicsQueue;

  // device, pool and queue could be grouped together and passed around

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

  if (g_validation) {
    vks::debug::freeDebugCallback(sInstance);
  }

  // THE END
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
GraphicsVk::createWindowRenderTarget(void* nativeWindow)
{
  return nullptr;
}

RenderTarget*
GraphicsVk::createImageRenderTarget(int width, int height, PixelFormat format)
{
  RenderTargetVk* rt = new RenderTargetVk(sVulkanDevice, sQueue, width, height, format);
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
  MeshVk* mesh =
    new MeshVk(sVulkanDevice, sCommandPool, sQueue, i_nVertices, i_Vertices, i_Normals, i_UVs, i_nIndices, i_Indices);
  return mesh;
}

VkShaderModule
GraphicsVk::loadShaderFromPtr(uint32_t* shaderCode, size_t size, VkDevice device)
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
