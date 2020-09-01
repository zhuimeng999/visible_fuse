//
// Created by lucius on 8/1/20.
//

#include "vulkan_utils/Vkd.h"
#include "vulkan_utils/Vkm.h"
#include "vulkan_utils/Vkp.h"
#include "vulkan_utils/Vkpc.h"
#include "vulkan_utils/Vkpg.h"
#include "vulkan_utils/Vkc.h"
#include "vulkan_utils/Vkb.h"
#include "vulkan_utils/Vktexture.h"
#include "log/log.h"
#include <fstream>

Vkc::Vkc(std::shared_ptr<Vkd> &pVkd) : pvkd(pVkd), commandPool(VK_NULL_HANDLE),
                                                          commandBuffer(VK_NULL_HANDLE), fence(VK_NULL_HANDLE){
  VkCommandPoolCreateInfo commandPoolCreateInfo;
  commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo.pNext = nullptr;
  commandPoolCreateInfo.queueFamilyIndex = pvkd->queueFamilyIndex;
  commandPoolCreateInfo.flags = 0;
  CHECK_VULKAN(vkCreateCommandPool(pvkd->device, &commandPoolCreateInfo, nullptr, &commandPool));

  VkCommandBufferAllocateInfo commandBufferAllocateInfo;
  commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.pNext = nullptr;
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.commandBufferCount = 1;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  CHECK_VULKAN(vkAllocateCommandBuffers(pvkd->device, &commandBufferAllocateInfo, &commandBuffer));

  VkFenceCreateInfo fenceCreateInfo;
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.pNext = nullptr;
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  CHECK_VULKAN(vkCreateFence(pvkd->device, &fenceCreateInfo, nullptr, &fence));
}

Vkc::~Vkc() {
  vkDestroyFence(pvkd->device, fence, nullptr);
  vkDestroyCommandPool(pvkd->device, commandPool, nullptr);
}

void Vkc::sumit() {
  CHECK_VULKAN(vkWaitForFences(pvkd->device, 1, &fence, true, 1000*1000*1000));
  CHECK_VULKAN(vkResetFences(pvkd->device, 1, &fence));

  const VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
  VkSubmitInfo submitInfo;
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pNext = nullptr;
  submitInfo.pCommandBuffers = &commandBuffer;
  submitInfo.commandBufferCount = 1;
  submitInfo.pWaitSemaphores = nullptr;
  submitInfo.waitSemaphoreCount = 0;
  submitInfo.pSignalSemaphores = nullptr;
  submitInfo.signalSemaphoreCount = 0;
  submitInfo.pWaitDstStageMask = &waitStageMask;
  CHECK_VULKAN(vkQueueSubmit(pvkd->queue, 1, &submitInfo, fence));
}

void Vkc::resourcePrepare(std::shared_ptr<Vkm> &pvkm) {
  // Barrier to ensure that input buffer transfer is finished before compute shader reads from it
  uploadBuffer(pvkm->paramsBuffer);
  uploadTexture(pvkm->refRGBTexture);
  uploadTexture(pvkm->refDepthProbTexture);
  for(auto i = 0; i < pvkm->srcRGBTextures.size(); i++){
    uploadTexture(pvkm->srcRGBTextures[i]);
    uploadTexture(pvkm->srcDepthProbTextures[i]);
  }
}

void Vkc::addComputePipeline(std::shared_ptr<Vkpc> &pvkpc, std::shared_ptr<Vkm> &pvkm, int scale) {
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pvkpc->pipelineLayout, 0, pvkm->descriptorSets.size(), pvkm->descriptorSets.data(), 0, 0);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pvkpc->pipeline);
  vkCmdDispatch(commandBuffer, pvkm->refRGBTexture->width*scale, pvkm->refRGBTexture->height*scale, 1);

//  generalBufferBarrier(pvkm->paramsBuffer);
//  generalBufferBarrier(pvkm->depthBuffer);
//  generalBufferBarrier(pvkm->probBuffer);
//  generalBufferBarrier(pvkm->scoreBuffer);
//
//  generalImageBarrier(pvkm->refRGBTexture);
//  generalImageBarrier(pvkm->refDepthProbTexture);
//  for(auto i = 0; i < pvkm->srcRGBTextures.size(); i++){
//    generalImageBarrier(pvkm->srcRGBTextures[i]);
//    generalImageBarrier(pvkm->srcDepthProbTextures[i]);
//  }
}

void Vkc::buildCommandBuffer(std::shared_ptr<Vkpc> &pvkpc, std::shared_ptr<Vkpg> &pvkpg, std::shared_ptr<Vkm> &pvkm)
{
  std::vector<VkClearValue> clearValues{
          { .color = { 0.0f, 0.0f, 0.0f, 1.0f } },
          { .color = { 0.0f, 0.0f, 0.0f, 0.0f } },
          { .depthStencil = {1.0f, 0} }
  };

  VkRenderPassBeginInfo renderPassBeginInfo;
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.pNext = nullptr;
  renderPassBeginInfo.renderPass = pvkpg->renderPass;
  renderPassBeginInfo.framebuffer = pvkpg->framebuffer;
  renderPassBeginInfo.renderArea.offset.x = 0;
  renderPassBeginInfo.renderArea.offset.y = 0;
  renderPassBeginInfo.renderArea.extent.width = pvkm->colorAttachment->width;
  renderPassBeginInfo.renderArea.extent.height = pvkm->colorAttachment->height;
  renderPassBeginInfo.clearValueCount = clearValues.size();
  renderPassBeginInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pvkpc->pipelineLayout, 0, pvkm->descriptorSets.size(), pvkm->descriptorSets.data(), 0, 0);
  VkViewport viewport;
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = pvkm->colorAttachment->width;
  viewport.height = pvkm->colorAttachment->height;
  viewport.minDepth = 0;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor;
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  scissor.extent.width = pvkm->colorAttachment->width;
  scissor.extent.height = pvkm->colorAttachment->height;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pvkpg->pipeline);
  uint32_t total_quat = (pvkm->colorAttachment->width - 1) * (pvkm->colorAttachment->height - 1);
  vkCmdDraw(commandBuffer, 6, total_quat, 0, 0);
  vkCmdEndRenderPass(commandBuffer);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pvkpc->pipelineLayout, 0, pvkm->descriptorSets.size(), pvkm->descriptorSets.data(), 0, 0);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pvkpc->pipeline);
  vkCmdDispatch(commandBuffer, pvkm->refRGBTexture->width, pvkm->refRGBTexture->height, 1);
}

void Vkc::resourceDownload(std::shared_ptr<Vkm> &pvkm) {
  downloadBuffer(pvkm->depthBuffer);
  downloadBuffer(pvkm->probBuffer);
  downloadBuffer(pvkm->visibleViewsBuffer);
//  VkImageSubresourceRange subresourceRange = {};
//  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//  subresourceRange.baseMipLevel = 0;
//  subresourceRange.levelCount = 1;
//  subresourceRange.layerCount = pvkm->colorAttachment->arrayLayers;
//  setImageLayout(pvkm->colorAttachment->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange, VK_SHADER_STAGE_COMPUTE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
//  subresourceRange.layerCount = pvkm->infoAttachment->arrayLayers;
//  setImageLayout(pvkm->infoAttachment->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange, VK_SHADER_STAGE_COMPUTE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
//  downloadTexture(pvkm->colorAttachment);
//  downloadTexture(pvkm->infoAttachment);
}

void Vkc::waitFinished() {
  CHECK_VULKAN(vkWaitForFences(pvkd->device, 1, &fence, VK_TRUE, UINT64_MAX));
  vkQueueWaitIdle(pvkd->queue);
  vkDeviceWaitIdle(pvkd->device);
}

void Vkc::setImageLayout(
        VkImage image,
        VkImageLayout oldImageLayout,
        VkImageLayout newImageLayout,
        VkImageSubresourceRange subresourceRange,
        VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask)
{
  // Create an image barrier object
  VkImageMemoryBarrier imageMemoryBarrier {};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.oldLayout = oldImageLayout;
  imageMemoryBarrier.newLayout = newImageLayout;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange = subresourceRange;

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old layout
  // before it will be transitioned to the new layout
  switch (oldImageLayout)
  {
    case VK_IMAGE_LAYOUT_UNDEFINED:
      // Image layout is undefined (or does not matter)
      // Only valid as initial layout
      // No flags required, listed only for completeness
      imageMemoryBarrier.srcAccessMask = 0;
      break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      // Image is preinitialized
      // Only valid as initial layout for linear images, preserves memory contents
      // Make sure host writes have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      // Image is a color attachment
      // Make sure any writes to the color buffer have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      // Image is a depth/stencil attachment
      // Make sure any writes to the depth/stencil buffer have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      // Image is a transfer source
      // Make sure any reads from the image have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      // Image is a transfer destination
      // Make sure any writes to the image have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      // Image is read by a shader
      // Make sure any shader reads from the image have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    default:
      // Other source layouts aren't handled (yet)
      break;
  }

  // Target layouts (new)
  // Destination access mask controls the dependency for the new image layout
  switch (newImageLayout)
  {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      // Image will be used as a transfer destination
      // Make sure any writes to the image have been finished
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      // Image will be used as a transfer source
      // Make sure any reads from the image have been finished
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      // Image will be used as a color attachment
      // Make sure any writes to the color buffer have been finished
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      // Image layout will be used as a depth/stencil attachment
      // Make sure any writes to depth/stencil buffer have been finished
      imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      // Image will be read in a shader (sampler, input attachment)
      // Make sure any writes to the image have been finished
      if (imageMemoryBarrier.srcAccessMask == 0)
      {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
      }
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    default:
      // Other source layouts aren't handled (yet)
      break;
  }

  // Put barrier inside setup command buffer
  vkCmdPipelineBarrier(
          commandBuffer,
          srcStageMask,
          dstStageMask,
          0,
          0, nullptr,
          0, nullptr,
          1, &imageMemoryBarrier);
}

void Vkc::generalImageBarrier(std::unique_ptr<Vktexture> &tex) {
  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.layerCount = tex->arrayLayers;
  setImageLayout(
          tex->image,
          VK_IMAGE_LAYOUT_GENERAL,
          VK_IMAGE_LAYOUT_GENERAL,
          subresourceRange, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
}

void Vkc::generalBufferBarrier(std::unique_ptr<Vkb> &buf) {
  VkBufferMemoryBarrier bufferMemoryBarrier;
  bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferMemoryBarrier.pNext = nullptr;
  bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.buffer = buf->stageBuffer;
  bufferMemoryBarrier.offset = 0;
  bufferMemoryBarrier.size = VK_WHOLE_SIZE;
  bufferMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  bufferMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
          VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
          0,
          0, nullptr,
          1, &bufferMemoryBarrier,
          0, nullptr);
}

void Vkc::uploadTexture(std::unique_ptr<Vktexture> &tex) {
  VkBufferMemoryBarrier bufferMemoryBarrier;
  bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferMemoryBarrier.pNext = nullptr;
  bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.buffer = tex->stageVkb->deviceBuffer;
  bufferMemoryBarrier.offset = 0;
  bufferMemoryBarrier.size = VK_WHOLE_SIZE;
  bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
  bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_HOST_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      1, &bufferMemoryBarrier,
      0, nullptr);

  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.layerCount = tex->arrayLayers;

  setImageLayout(
      tex->image,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      subresourceRange, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

  VkBufferImageCopy bufferImageCopy;
  bufferImageCopy.bufferOffset = 0;
  bufferImageCopy.bufferImageHeight = 0;
  bufferImageCopy.bufferRowLength = 0;
  bufferImageCopy.imageExtent.width =  tex->width;
  bufferImageCopy.imageExtent.height =  tex->height;
  bufferImageCopy.imageExtent.depth =  1;
  bufferImageCopy.imageOffset.x = 0;
  bufferImageCopy.imageOffset.y = 0;
  bufferImageCopy.imageOffset.z = 0;
  bufferImageCopy.imageSubresource.baseArrayLayer = 0;
  bufferImageCopy.imageSubresource.layerCount = tex->arrayLayers;
  bufferImageCopy.imageSubresource.mipLevel = 0;
  bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vkCmdCopyBufferToImage(commandBuffer, tex->stageVkb->deviceBuffer, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferImageCopy);
  setImageLayout(
      tex->image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL,
      subresourceRange, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

}

void Vkc::downloadTexture(std::unique_ptr<Vktexture> &tex) {
  VkBufferImageCopy bufferImageCopy;
  bufferImageCopy.bufferOffset = 0;
  bufferImageCopy.bufferImageHeight = 0;
  bufferImageCopy.bufferRowLength = 0;
  bufferImageCopy.imageExtent.width =  tex->width;
  bufferImageCopy.imageExtent.height =  tex->height;
  bufferImageCopy.imageExtent.depth =  1;
  bufferImageCopy.imageOffset.x = 0;
  bufferImageCopy.imageOffset.y = 0;
  bufferImageCopy.imageOffset.z = 0;
  bufferImageCopy.imageSubresource.baseArrayLayer = 0;
  bufferImageCopy.imageSubresource.layerCount = tex->arrayLayers;
  bufferImageCopy.imageSubresource.mipLevel = 0;
  bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vkCmdCopyImageToBuffer(commandBuffer, tex->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tex->stageVkb->deviceBuffer, 1,
                         &bufferImageCopy);

  VkBufferMemoryBarrier bufferMemoryBarrier;
  bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferMemoryBarrier.pNext = nullptr;
  bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.buffer = tex->stageVkb->deviceBuffer;
  bufferMemoryBarrier.offset = 0;
  bufferMemoryBarrier.size = VK_WHOLE_SIZE;
  bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  bufferMemoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(
          commandBuffer,
          VK_PIPELINE_STAGE_TRANSFER_BIT,
          VK_PIPELINE_STAGE_HOST_BIT,
          0,
          0, nullptr,
          1, &bufferMemoryBarrier,
          0, nullptr);

}

void Vkc::uploadBuffer(std::unique_ptr<Vkb> &buf) {
  VkBufferMemoryBarrier bufferMemoryBarrier;
  bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferMemoryBarrier.pNext = nullptr;
  bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.buffer = buf->stageBuffer;
  bufferMemoryBarrier.offset = 0;
  bufferMemoryBarrier.size = VK_WHOLE_SIZE;
  bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
  bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_HOST_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      1, &bufferMemoryBarrier,
      0, nullptr);

  VkBufferCopy bufferCopy;
  bufferCopy.srcOffset = 0;
  bufferCopy.dstOffset = 0;
  bufferCopy.size = buf->deviceSize;
  vkCmdCopyBuffer(commandBuffer, buf->stageBuffer, buf->deviceBuffer, 1, &bufferCopy);

  bufferMemoryBarrier.buffer = buf->deviceBuffer;
  bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  bufferMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      0,
      0, nullptr,
      1, &bufferMemoryBarrier,
      0, nullptr);
}

void Vkc::downloadBuffer(std::unique_ptr<Vkb> &buf) {
  VkBufferMemoryBarrier bufferMemoryBarrier;
  bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  bufferMemoryBarrier.pNext = nullptr;
  bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  bufferMemoryBarrier.buffer = buf->deviceBuffer;
  bufferMemoryBarrier.offset = 0;
  bufferMemoryBarrier.size = VK_WHOLE_SIZE;
  bufferMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      0,
      0, nullptr,
      1, &bufferMemoryBarrier,
      0, nullptr);

  VkBufferCopy bufferCopy;
  bufferCopy.srcOffset = 0;
  bufferCopy.dstOffset = 0;
  bufferCopy.size = buf->deviceSize;
  vkCmdCopyBuffer(commandBuffer, buf->deviceBuffer, buf->stageBuffer, 1, &bufferCopy);

  bufferMemoryBarrier.buffer = buf->stageBuffer;
  bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  bufferMemoryBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_HOST_BIT,
      0,
      0, nullptr,
      1, &bufferMemoryBarrier,
      0, nullptr);
}


