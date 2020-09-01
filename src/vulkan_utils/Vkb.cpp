//
// Created by lucius on 8/10/20.
//

#include "vulkan_utils/Vkd.h"
#include "vulkan_utils/Vkb.h"
#include "log/log.h"

Vkb::Vkb(std::shared_ptr<Vkd> &pVkd, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags, bool createStage) :
                                                              pvkd(pVkd), deviceSize(size),
                                                              bufferUsageFlags(usageFlags),
                                                              memoryPropertyFlags(propertyFlags){
  createBuffer(deviceBuffer, deviceMemory, size, bufferUsageFlags, memoryPropertyFlags);
  if(createStage){
    VkBufferUsageFlags stageFlags = 0;
    if(bufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT){
      stageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if(bufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT){
      stageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    createBuffer(stageBuffer, stageDeviceMemory, size, stageFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }
}

Vkb::~Vkb() {
  if(deviceBuffer != VK_NULL_HANDLE){
    vkDestroyBuffer(pvkd->device, deviceBuffer, nullptr);
  }
  if(deviceMemory != VK_NULL_HANDLE){
    vkFreeMemory(pvkd->device, deviceMemory, nullptr);
  }
  if(stageBuffer != VK_NULL_HANDLE){
    vkDestroyBuffer(pvkd->device, stageBuffer, nullptr);
  }
  if(stageDeviceMemory != VK_NULL_HANDLE){
    vkFreeMemory(pvkd->device, stageDeviceMemory, nullptr);
  }
}

void Vkb::createBuffer(VkBuffer &buffer, VkDeviceMemory &memory, VkDeviceSize size, VkBufferUsageFlags usageFlags,
                       VkMemoryPropertyFlags propertyFlags) {
  VkBufferCreateInfo bufferCreateInfo;
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.pNext = nullptr;
  bufferCreateInfo.flags = 0;
  bufferCreateInfo.pQueueFamilyIndices = &pvkd->queueFamilyIndex;
  bufferCreateInfo.queueFamilyIndexCount = 1;
  bufferCreateInfo.size = size;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bufferCreateInfo.usage = usageFlags;
  CHECK_VULKAN(vkCreateBuffer(pvkd->device, &bufferCreateInfo, nullptr, &buffer));

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(pvkd->device, buffer, &memoryRequirements);
  VkMemoryAllocateInfo memoryAllocateInfo;
  memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocateInfo.pNext = nullptr;
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = pvkd->getMemoryType(memoryRequirements.memoryTypeBits, propertyFlags);
  CHECK_VULKAN(vkAllocateMemory(pvkd->device, &memoryAllocateInfo, nullptr, &memory));
  CHECK_VULKAN(vkBindBufferMemory(pvkd->device, buffer, memory, 0));
}

void Vkb::uploadData(const void *data, VkDeviceSize offset, VkDeviceSize len) {
  VkDeviceMemory mem;
  VkMemoryPropertyFlags memProp;
  if(stageDeviceMemory){
    mem = stageDeviceMemory;
    memProp = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  } else {
    mem = deviceMemory;
    CHECK(memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    memProp = memoryPropertyFlags;
  }
  void *mapped;
  CHECK_VULKAN(vkMapMemory(pvkd->device, mem, offset, len, 0, &mapped));
  memcpy(mapped, data, len);
  if(!(memProp | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){
    VkMappedMemoryRange mappedRange;
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.pNext = nullptr;
    mappedRange.memory = mem;
    mappedRange.offset = offset;
    mappedRange.size = len;
    vkFlushMappedMemoryRanges(pvkd->device, 1, &mappedRange);
  }
  vkUnmapMemory(pvkd->device, mem);
}

void Vkb::downloadData(void *data, VkDeviceSize offset, VkDeviceSize len) {
  VkDeviceMemory mem;
  VkMemoryPropertyFlags memProp;
  if(stageDeviceMemory){
    mem = stageDeviceMemory;
    memProp = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  } else {
    mem = deviceMemory;
    CHECK(memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    memProp = memoryPropertyFlags;
  }

  void *mapped;
  CHECK_VULKAN(vkMapMemory(pvkd->device, mem, offset, len, 0, &mapped));
  if(memProp | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT){
    /* do nothing */
  } else {
    VkMappedMemoryRange mappedRange;
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.pNext = nullptr;
    mappedRange.memory = mem;
    mappedRange.offset = offset;
    mappedRange.size = len;
    vkInvalidateMappedMemoryRanges(pvkd->device, 1, &mappedRange);
  }
  memcpy(data, mapped, len);
  vkUnmapMemory(pvkd->device, mem);
}
