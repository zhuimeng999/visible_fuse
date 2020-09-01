//
// Created by lucius on 8/1/20.
//

#ifndef UNTITLED_VKC_H
#define UNTITLED_VKC_H

#include <vulkan/vulkan.hpp>

class Vkd;
class Vkm;
class Vkp;
class Vkpc;
class Vkpg;
class Vkb;
class Vktexture;
class Vkc {
public:
  explicit Vkc(std::shared_ptr<Vkd> &pVkd);
  ~Vkc();

  inline void beginCommandBuffer() const {
    VkCommandBufferBeginInfo commandBufferBeginInfo;
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pNext = nullptr;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    commandBufferBeginInfo.flags = 0;
    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
  }

  void resourcePrepare(std::shared_ptr<Vkm> &pvkm);
  void addComputePipeline(std::shared_ptr<Vkpc> &pvkpc, std::shared_ptr<Vkm> &pvkm, int scale = 1);
  void buildCommandBuffer(std::shared_ptr<Vkpc> &pvkpc, std::shared_ptr<Vkpg> &pvkpg, std::shared_ptr<Vkm> &pvkm);
  void resourceDownload(std::shared_ptr<Vkm> &pvkm);
  inline void endCommandBuffer() const {
    vkEndCommandBuffer(commandBuffer);
  };

  void sumit();
  void waitFinished();

  void generalImageBarrier(std::unique_ptr<Vktexture> &tex);
  void generalBufferBarrier(std::unique_ptr<Vkb> &buf);

  void setImageLayout(
          VkImage image,
          VkImageLayout oldImageLayout,
          VkImageLayout newImageLayout,
          VkImageSubresourceRange subresourceRange,
          VkPipelineStageFlags srcStageMask,
          VkPipelineStageFlags dstStageMask);

  void uploadTexture(std::unique_ptr<Vktexture> & tex);
  void downloadTexture(std::unique_ptr<Vktexture> & tex);
  void uploadBuffer(std::unique_ptr<Vkb> &buf);
  void downloadBuffer(std::unique_ptr<Vkb> &buf);
public:
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffer;
  VkFence fence;

private:
  std::shared_ptr<Vkd> pvkd;
};


#endif //UNTITLED_VKC_H
