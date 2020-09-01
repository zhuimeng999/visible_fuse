//
// Created by lucius on 8/10/20.
//

#include "vulkan_utils/Vkd.h"
#include "vulkan_utils/Vkb.h"
#include "vulkan_utils/Vktexture.h"
#include "log/log.h"

Vktexture::Vktexture(std::shared_ptr<Vkd> &pVkd, VkDeviceSize img_width, VkDeviceSize img_height,
                     VkImageUsageFlags imageUsageFlags, VkFormat img_format, VkImageLayout img_layout,
                     bool createStage)
    : pvkd(pVkd),
      width(img_width),
      height(img_height),
      format(img_format),
      imageLayout(img_layout) {
  if((imageUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) or (imageUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)){
    arrayLayers = 10;
  } else {
    arrayLayers = 1;
  }

  VkImageCreateInfo imageCreateInfo;
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.pNext = nullptr;
  imageCreateInfo.flags = 0;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format;
  imageCreateInfo.extent.width = img_width;
  imageCreateInfo.extent.height = img_height;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = arrayLayers;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage = imageUsageFlags;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.queueFamilyIndexCount = pvkd->queueFamilyIndex;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  CHECK_VULKAN(vkCreateImage(pvkd->device, &imageCreateInfo, nullptr, &image));

  VkMemoryRequirements memoryRequirements;
  vkGetImageMemoryRequirements(pvkd->device, image, &memoryRequirements);
  VkMemoryAllocateInfo memoryAllocateInfo;
  memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocateInfo.pNext = nullptr;
  memoryAllocateInfo.memoryTypeIndex = pvkd->getMemoryType(memoryRequirements.memoryTypeBits,
                                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  CHECK_VULKAN(vkAllocateMemory(pvkd->device, &memoryAllocateInfo, nullptr, &deviceMemory));
  CHECK_VULKAN(vkBindImageMemory(pvkd->device, image, deviceMemory, 0));

  VkComponentMapping componentMapping{};
  VkImageSubresourceRange imageSubresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, arrayLayers};
  if(imageUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
    if(img_format & VK_FORMAT_D32_SFLOAT_S8_UINT){
      imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    } else if(img_format & VK_FORMAT_D32_SFLOAT) {
      imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
      CHECK(!"unsupport image");
    }
  }
  VkImageViewCreateInfo imageViewCreateInfo;
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreateInfo.pNext = nullptr;
  imageViewCreateInfo.flags = 0;
  imageViewCreateInfo.image = image;
  imageViewCreateInfo.format = format;
  if(arrayLayers > 1){
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
  } else {
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  }
  imageViewCreateInfo.components = componentMapping;
  imageViewCreateInfo.subresourceRange = imageSubresourceRange;
  CHECK_VULKAN(vkCreateImageView(pvkd->device, &imageViewCreateInfo, nullptr, &imageView));

  if (createStage) {
    VkBufferUsageFlags usageFlags = 0;
    if (imageUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
      usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (imageUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
      usageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if (format == VK_FORMAT_R32G32_SFLOAT) {
      stageVkb = std::make_unique<Vkb>(pvkd, width * height * arrayLayers * 8, usageFlags,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       false);
    } else if(format == VK_FORMAT_R32G32B32A32_SFLOAT){
      stageVkb = std::make_unique<Vkb>(pvkd, width * height * arrayLayers * 16, usageFlags,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       false);
    }else if ((format == VK_FORMAT_R8G8B8A8_UINT) | (format == VK_FORMAT_R8G8B8A8_UNORM)) {
      stageVkb = std::make_unique<Vkb>(pvkd, width * height * arrayLayers * 4, usageFlags,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       false);
    } else {
      CHECK(!"unknown format");
    }
  }

  if(imageUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT){
    VkSamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = nullptr;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    samplerCreateInfo.mipLodBias = 0.0;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.maxAnisotropy = 0.0;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.minLod = 0;
    samplerCreateInfo.maxLod = 0;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    samplerCreateInfo.unnormalizedCoordinates = VK_TRUE;
    CHECK_VULKAN(vkCreateSampler(pvkd->device, &samplerCreateInfo, nullptr, &sampler));
  }
}

Vktexture::~Vktexture() {
  vkDestroyImage(pvkd->device, image, nullptr);
  vkFreeMemory(pvkd->device, deviceMemory, nullptr);
  vkDestroyImageView(pvkd->device, imageView, nullptr);
  vkDestroySampler(pvkd->device, sampler, nullptr);
}

void Vktexture::uploadData(const void *data, VkDeviceSize offset, VkDeviceSize len) {
  stageVkb->uploadData(data, offset, len);
}

void Vktexture::downloadData(void *data, VkDeviceSize offset, VkDeviceSize len) {
  stageVkb->downloadData(data, offset, len);
}

void Vktexture::uploadDepthProb(const cv::Mat &depth, const cv::Mat &prob) {
  void *mapped;
  CHECK_VULKAN(vkMapMemory(pvkd->device, stageVkb->deviceMemory, 0, stageVkb->deviceSize, 0, &mapped));
  for (auto i = 0; i < depth.rows * depth.cols; i++) {
    static_cast<float *>(mapped)[2 * i] = ((float *) depth.data)[i];
    static_cast<float *>(mapped)[2 * i + 1] = ((float *) prob.data)[i];
  }

  if (!(stageVkb->memoryPropertyFlags | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
    VkMappedMemoryRange mappedRange;
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.pNext = nullptr;
    mappedRange.memory = stageVkb->deviceMemory;
    mappedRange.offset = 0;
    mappedRange.size = stageVkb->deviceSize;
    vkFlushMappedMemoryRanges(pvkd->device, 1, &mappedRange);
  }
  vkUnmapMemory(pvkd->device, stageVkb->deviceMemory);
}

void Vktexture::downloadDepthProb(const cv::Mat &depth, const cv::Mat &prob) {

}
