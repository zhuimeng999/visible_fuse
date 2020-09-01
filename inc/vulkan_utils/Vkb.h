//
// Created by lucius on 8/10/20.
//

#ifndef UNTITLED_VKB_H
#define UNTITLED_VKB_H

#include <vulkan/vulkan.hpp>
#include <opencv2/core.hpp>

class Vkd;
class Vkb {
public:
  explicit Vkb(std::shared_ptr<Vkd> &pVkd, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, bool createStage);
  ~Vkb();

  void uploadData(const void *data, VkDeviceSize offset, VkDeviceSize len);
  void downloadData(void *data, VkDeviceSize offset, VkDeviceSize len);

  VkDeviceSize deviceSize;
  VkBufferUsageFlags bufferUsageFlags;
  VkMemoryPropertyFlags memoryPropertyFlags;
  VkBuffer deviceBuffer = VK_NULL_HANDLE;
  VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
  VkBuffer stageBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stageDeviceMemory = VK_NULL_HANDLE;

private:
  std::shared_ptr<Vkd> pvkd;
  void createBuffer(VkBuffer &buffer, VkDeviceMemory &memory, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);
};


#endif //UNTITLED_VKB_H
