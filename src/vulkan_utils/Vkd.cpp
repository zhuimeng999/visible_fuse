//
// Created by lucius on 8/1/20.
//

#include "vulkan_utils/Vki.h"
#include "vulkan_utils/Vkd.h"
#include "log/log.h"

Vkd::Vkd(std::shared_ptr<Vki> &pVki) : pvki(pVki), queueFamilyIndex(UINT32_MAX),
                                            phyDevice(VK_NULL_HANDLE),
                                            device(VK_NULL_HANDLE),
                                            queue(VK_NULL_HANDLE)
{
  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;
  deviceFeatures.shaderStorageImageMultisample = VK_TRUE;
  deviceFeatures.geometryShader = VK_TRUE;
  deviceFeatures.tessellationShader = VK_TRUE;
  deviceFeatures.fillModeNonSolid = VK_TRUE;

  std::vector<VkDeviceQueueCreateInfo> findedQueueFamilies;
  float queuePriority = 1.0f;
  for(const auto &it: pvki->phyDevices){
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(it, &physicalDeviceProperties);
    if(physicalDeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
      continue;
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures(it, &physicalDeviceFeatures);
    if(physicalDeviceFeatures.tessellationShader and physicalDeviceFeatures.samplerAnisotropy
                                                  and physicalDeviceFeatures.fillModeNonSolid
                                                  and physicalDeviceFeatures.shaderStorageImageMultisample
                                                  and physicalDeviceFeatures.geometryShader) {


      uint32_t phyQueueFamilyCount = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(it, &phyQueueFamilyCount, nullptr);
      std::vector<VkQueueFamilyProperties> phyQueueFamilies(phyQueueFamilyCount);
      vkGetPhysicalDeviceQueueFamilyProperties(it, &phyQueueFamilyCount, phyQueueFamilies.data());

      for (auto i = 0; i < phyQueueFamilyCount; i++) {
        if ((phyQueueFamilies[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
            (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) {
          queueFamilyIndex = i;
          break;
        }
      }
      if (queueFamilyIndex != UINT32_MAX) {
        VkDeviceQueueCreateInfo deviceQueueCreateInfo;
        deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo.pNext = nullptr;
        deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        deviceQueueCreateInfo.flags = 0;
        findedQueueFamilies.push_back(deviceQueueCreateInfo);
        phyDevice = it;
        break;
      }
    }
  }
  CHECK(phyDevice != VK_NULL_HANDLE);

  VkDeviceCreateInfo deviceCreateInfo;
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pNext = nullptr;
  deviceCreateInfo.pQueueCreateInfos = findedQueueFamilies.data();
  deviceCreateInfo.queueCreateInfoCount = findedQueueFamilies.size();
  deviceCreateInfo.ppEnabledLayerNames = nullptr;
  deviceCreateInfo.enabledLayerCount = 0;
  deviceCreateInfo.ppEnabledExtensionNames = nullptr;
  deviceCreateInfo.enabledExtensionCount = 0;
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
  deviceCreateInfo.flags = 0;
  CHECK_VULKAN(vkCreateDevice(phyDevice, &deviceCreateInfo, nullptr, &device));

  vkGetPhysicalDeviceMemoryProperties(phyDevice, &physicalDeviceMemoryProperties);
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
  getMaxUsableSampleCount();

//  /*
//  Display multi view features and properties
//*/
//
//  VkPhysicalDeviceFeatures2KHR deviceFeatures2{};
//  VkPhysicalDeviceMultiviewFeaturesKHR extFeatures{};
//  extFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
//  deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
//  deviceFeatures2.pNext = &extFeatures;
//  PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(pvki->instance, "vkGetPhysicalDeviceFeatures2KHR"));
//  vkGetPhysicalDeviceFeatures2KHR(phyDevice, &deviceFeatures2);
//  BOOST_LOG_TRIVIAL(info) << "Multiview features:";
//  BOOST_LOG_TRIVIAL(info) << "\tmultiview = " << extFeatures.multiview;
//  BOOST_LOG_TRIVIAL(info) << "\tmultiviewGeometryShader = " << extFeatures.multiviewGeometryShader;
//  BOOST_LOG_TRIVIAL(info) << "\tmultiviewTessellationShader = " << extFeatures.multiviewTessellationShader;
//
//  VkPhysicalDeviceProperties2KHR deviceProps2{};
//  VkPhysicalDeviceMultiviewPropertiesKHR extProps{};
//  extProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR;
//  deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
//  deviceProps2.pNext = &extProps;
//  PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(pvki->instance, "vkGetPhysicalDeviceProperties2KHR"));
//  vkGetPhysicalDeviceProperties2KHR(phyDevice, &deviceProps2);
//  BOOST_LOG_TRIVIAL(info) << "Multiview properties:";
//  BOOST_LOG_TRIVIAL(info) << "\tmaxMultiviewViewCount = " << extProps.maxMultiviewViewCount;
//  BOOST_LOG_TRIVIAL(info) << "\tmaxMultiviewInstanceIndex = " << extProps.maxMultiviewInstanceIndex;
}

Vkd::~Vkd() {
  vkDestroyDevice(device, nullptr);
}

uint32_t Vkd::getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties) {
  for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }
    typeBits >>= 1;
  }
  BOOST_LOG_TRIVIAL(fatal) << "can not find proper memory type";
  exit(EXIT_FAILURE);
}

VkFormat Vkd::getSupportedDepthFormat()
{
  // Since all depth formats may be optional, we need to find a suitable depth format to use
  // Start with the highest precision packed format
  std::vector<VkFormat> depthFormats = {
      VK_FORMAT_D32_SFLOAT_S8_UINT,
      VK_FORMAT_D32_SFLOAT
  };

  for (auto& format : depthFormats)
  {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(phyDevice, format, &formatProps);
    // Format must support depth stencil attachment for optimal tiling
    if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) and
            (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
    {
      return format;
    }
  }

  BOOST_LOG_TRIVIAL(fatal) << "can not find proper depth format";
  exit(EXIT_FAILURE);
}

VkSampleCountFlagBits Vkd::getMaxUsableSampleCount() {
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(phyDevice, &physicalDeviceProperties);

  VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
  if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
  if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
  if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
  if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
  if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
  if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

  return VK_SAMPLE_COUNT_1_BIT;
}