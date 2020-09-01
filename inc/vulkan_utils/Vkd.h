//
// Created by lucius on 8/1/20.
//

#ifndef UNTITLED_VKD_H
#define UNTITLED_VKD_H

#include <vulkan/vulkan.hpp>

class Vki;

class Vkd {
public:
  explicit Vkd(std::shared_ptr<Vki> &pVki);
  ~Vkd();

  uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties);
  VkFormat getSupportedDepthFormat();
  VkSampleCountFlagBits getMaxUsableSampleCount();

public:
  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
  uint32_t queueFamilyIndex;
  VkPhysicalDevice phyDevice;
  VkDevice device;
  VkQueue queue;

private:
  std::shared_ptr<Vki> pvki;
};


#endif //UNTITLED_VKD_H
