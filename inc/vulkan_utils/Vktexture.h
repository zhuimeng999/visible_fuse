//
// Created by lucius on 8/10/20.
//

#ifndef UNTITLED_VKIV_H
#define UNTITLED_VKIV_H

#include <vulkan/vulkan.hpp>
#include <opencv2/core.hpp>

class Vkd;

class Vkb;

class Vktexture {
public:
  Vktexture(std::shared_ptr<Vkd> &pVkd, VkDeviceSize img_width, VkDeviceSize img_height,
            VkImageUsageFlags imageUsageFlags, VkFormat img_format, VkImageLayout img_layout,
            bool createStage);

  ~Vktexture();

  void uploadData(const void *data, VkDeviceSize offset, VkDeviceSize len);

  void downloadData(void *data, VkDeviceSize offset, VkDeviceSize len);

  void uploadDepthProb(const cv::Mat &depth, const cv::Mat &prob);

  void downloadDepthProb(const cv::Mat &depth, const cv::Mat &prob);


  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
  VkImageView imageView = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkDeviceSize width;
  VkDeviceSize height;
  uint32_t arrayLayers;
  VkFormat format;
  VkImageLayout imageLayout;

  std::unique_ptr<Vkb> stageVkb;

private:
  std::shared_ptr<Vkd> pvkd;

};


#endif //UNTITLED_VKIV_H
