//
// Created by lucius on 8/1/20.
//

#include "vulkan_utils/Vki.h"
#include "log/log.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t object,
    size_t location,
    int32_t messageCode,
    const char* pLayerPrefix,
    const char* pMessage,
    void* pUserData)
{
  if(flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT){
    BOOST_LOG_TRIVIAL(debug) << pLayerPrefix << ", " << pMessage;
  } else if(flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT){
    BOOST_LOG_TRIVIAL(info) << pLayerPrefix << ", " << pMessage;
  } else if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT){
    BOOST_LOG_TRIVIAL(warning) << pLayerPrefix << ", " << pMessage;
  } else if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT){
    BOOST_LOG_TRIVIAL(warning) << pLayerPrefix << ", " << pMessage;
  } else if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT){
    BOOST_LOG_TRIVIAL(error) << pLayerPrefix << ", " << pMessage;
    exit(EXIT_FAILURE);
  } else {
    BOOST_LOG_TRIVIAL(fatal) << pLayerPrefix << ", " << pMessage;
    exit(EXIT_FAILURE);
  }
  return VK_FALSE;
}

Vki::Vki(bool enableValidate) : instance() , isValidateEnabled(enableValidate), debugReportCallback(){
  VkApplicationInfo  applicationInfo;
  applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  applicationInfo.pNext = nullptr;
  applicationInfo.pApplicationName = "test";
  applicationInfo.applicationVersion = 1;
  applicationInfo.pEngineName = "test";
  applicationInfo.engineVersion = 1;
  applicationInfo.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo instanceCreateInfo;
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pNext = nullptr;
  instanceCreateInfo.pApplicationInfo = &applicationInfo;

  std::vector<const char *> enabledLayers;
  if(enableValidate){
    enabledLayers.push_back("VK_LAYER_KHRONOS_validation");
  }
  uint32_t availableLayerCount;
  CHECK_VULKAN(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));
  availableLayers.resize(availableLayerCount);
  CHECK_VULKAN(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()));
  CHECK(isLayerAvailable(enabledLayers));

  instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();
  instanceCreateInfo.enabledLayerCount = enabledLayers.size();

  std::vector<const char *> enabledExtensions;
  if(enableValidate){
    enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  }
//  enabledExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
  instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
  instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
  instanceCreateInfo.flags = 0;
  CHECK_VULKAN(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

  if (enableValidate) {
    VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo;
    debugReportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    debugReportCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                    VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    debugReportCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugMessageCallback;
    debugReportCreateInfo.pUserData = this;

    // We have to explicitly load this function.
    auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
    CHECK(vkCreateDebugReportCallbackEXT);
    CHECK_VULKAN(vkCreateDebugReportCallbackEXT(instance, &debugReportCreateInfo, nullptr, &debugReportCallback));
  }

  uint32_t phyDeviceCount = 0;
  CHECK_VULKAN(vkEnumeratePhysicalDevices(instance, &phyDeviceCount, nullptr));
  phyDevices.resize(phyDeviceCount);
  CHECK_VULKAN(vkEnumeratePhysicalDevices(instance, &phyDeviceCount, phyDevices.data()));
}

Vki::~Vki() {
  if (isValidateEnabled) {
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
    CHECK(vkDestroyDebugReportCallback);
    vkDestroyDebugReportCallback(instance, debugReportCallback, nullptr);
  }
  vkDestroyInstance(instance, nullptr);
}

inline bool Vki::isLayerAvailable(std::vector<const char *> &requestLayer){
  uint32_t availableCount = 0;
  for(const char * it: requestLayer){
    for(const auto &availableLayer: availableLayers){
      if(!strcmp(it, availableLayer.layerName)){
        ++availableCount;
        break;
      }
    }
  }

  return availableCount == requestLayer.size();
}
