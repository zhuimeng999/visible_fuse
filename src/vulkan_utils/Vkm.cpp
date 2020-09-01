//
// Created by lucius on 8/1/20.
//

#include "vulkan_utils/Vkd.h"
#include "vulkan_utils/Vkb.h"
#include "vulkan_utils/Vktexture.h"
#include "vulkan_utils/Vkm.h"
#include "vulkan_utils/Vkc.h"
#include "log/log.h"
#include <opencv2/imgproc.hpp>

inline static Eigen::Matrix4f getRelativePose(const Eigen::Matrix3d &ref_K, const Eigen::Isometry3d &ref_pose, const Eigen::Matrix3d &src_K, const Eigen::Isometry3d &src_pose){
  const auto &&t1 = ref_pose*src_pose.inverse();
  Eigen::Matrix4d res = Eigen::Matrix4d::Identity();
  res.block(0, 0, 3, 3) = ref_K*t1.rotation()*src_K.inverse();
  res.block(0, 3, 3, 1) = ref_K*t1.translation();
//  Eigen::Matrix4d res = src_pose.inverse()*Eigen::Matrix4d::Identity();
//  res.block(0, 0, 3, 3) = src_K.inverse();
  return res.cast<float>();
}

inline static Eigen::Matrix4f getInversePose(const Eigen::Matrix3d &ref_K, const Eigen::Isometry3d &ref_pose){
  const auto &&t1 = ref_pose.inverse();
  Eigen::Matrix4d res = Eigen::Matrix4d::Identity();
  res.block(0, 0, 3, 3) = t1.rotation()*ref_K.inverse();
  res.block(0, 3, 3, 1) = t1.translation();

  return res.cast<float>();
}

Vkm::Vkm(std::shared_ptr<Vkd> &pVkd, VkDeviceSize maxWidth, VkDeviceSize maxHeight) : pvkd(pVkd){
  buildDescriptorSetInfos();
  buildDescriptorSets();
  colorAttachment = std::make_unique<Vktexture>(pvkd, maxWidth, maxHeight, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, true);
  infoAttachment = std::make_unique<Vktexture>(pvkd, maxWidth, maxHeight, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, true);
  depthAttachment = std::make_unique<Vktexture>(pvkd, maxWidth, maxHeight, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_LAYOUT_UNDEFINED, false);
}

Vkm::~Vkm() {
//  vkFreeDescriptorSets(pvkd->device, descriptorPool, descriptorSets.size(), descriptorSets.data());
  for(auto &it: descriptorSetLayouts){
    vkDestroyDescriptorSetLayout(pvkd->device, it, nullptr);
  }
  vkDestroyDescriptorPool(pvkd->device, descriptorPool, nullptr);
}

void Vkm::buildDescriptorSetInfos() {
  auto & setInfos0 = descSetInfos.emplace_back();
  auto & params = setInfos0.emplace_back();
  params.binding = 0;
  params.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  params.descriptorCount = 1;
  params.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
  params.pImmutableSamplers = nullptr;

  auto & depth = setInfos0.emplace_back();
  depth.binding = 1;
  depth.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  depth.descriptorCount = 1;
  depth.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  depth.pImmutableSamplers = nullptr;

  auto & prob = setInfos0.emplace_back();
  prob.binding = 2;
  prob.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  prob.descriptorCount = 1;
  prob.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  prob.pImmutableSamplers = nullptr;

  auto & score = setInfos0.emplace_back();
  score.binding = 3;
  score.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  score.descriptorCount = 1;
  score.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  score.pImmutableSamplers = nullptr;

  auto & ref_rgb = setInfos0.emplace_back();
  ref_rgb.binding = 4;
  ref_rgb.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  ref_rgb.descriptorCount = 1;
  ref_rgb.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  ref_rgb.pImmutableSamplers = nullptr;

  auto & ref_depth_prob = setInfos0.emplace_back();
  ref_depth_prob.binding = 5;
  ref_depth_prob.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  ref_depth_prob.descriptorCount = 1;
  ref_depth_prob.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  ref_depth_prob.pImmutableSamplers = nullptr;

  auto & src_rgbs = setInfos0.emplace_back();
  src_rgbs.binding = 6;
  src_rgbs.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  src_rgbs.descriptorCount = 10;
  src_rgbs.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
  src_rgbs.pImmutableSamplers = nullptr;

  auto & src_depth_probs = setInfos0.emplace_back();
  src_depth_probs.binding = 7;
  src_depth_probs.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  src_depth_probs.descriptorCount = 10;
  src_depth_probs.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
  src_depth_probs.pImmutableSamplers = nullptr;

  auto & proj_rgbs = setInfos0.emplace_back();
  proj_rgbs.binding = 8;
  proj_rgbs.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  proj_rgbs.descriptorCount = 1;
  proj_rgbs.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  proj_rgbs.pImmutableSamplers = nullptr;

  auto & proj_depth_probs = setInfos0.emplace_back();
  proj_depth_probs.binding = 9;
  proj_depth_probs.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  proj_depth_probs.descriptorCount = 1;
  proj_depth_probs.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  proj_depth_probs.pImmutableSamplers = nullptr;
}

void Vkm::buildDescriptorSets() {
  std::vector<VkDescriptorPoolSize> descriptorPoolSize;
  for(const auto &it: descSetInfos){
    for(const auto &binding : it){
      int i = 0;
      for(; i < descriptorPoolSize.size();i++){
        auto &poolSizeIt = descriptorPoolSize[i];
        if(poolSizeIt.type == binding.descriptorType){
          poolSizeIt.descriptorCount += binding.descriptorCount;
          break;
        }
      }
      if(i == descriptorPoolSize.size()){
        descriptorPoolSize.push_back({.type = binding.descriptorType, .descriptorCount = binding.descriptorCount});
      }
    }
  }

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
  descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.pNext = nullptr;
  descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSize.data();
  descriptorPoolCreateInfo.poolSizeCount = descriptorPoolSize.size();
  descriptorPoolCreateInfo.maxSets = descSetInfos.size();
  descriptorPoolCreateInfo.flags = 0;
  CHECK_VULKAN(vkCreateDescriptorPool(pvkd->device, &descriptorPoolCreateInfo, nullptr, &descriptorPool));

  descriptorSetLayouts.resize(descSetInfos.size());
  for(auto i = 0; i < descSetInfos.size(); i++){
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.pBindings = descSetInfos[i].data();
    descriptorSetLayoutCreateInfo.bindingCount = descSetInfos[i].size();
    descriptorSetLayoutCreateInfo.flags = 0;
    CHECK_VULKAN(vkCreateDescriptorSetLayout(pvkd->device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts[i]));
  }

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
  descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.pNext = nullptr;
  descriptorSetAllocateInfo.descriptorPool = descriptorPool;
  descriptorSetAllocateInfo.descriptorSetCount = descriptorSetLayouts.size();
  descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();
  descriptorSets.resize(descriptorSetLayouts.size());
  CHECK_VULKAN(vkAllocateDescriptorSets(pvkd->device, &descriptorSetAllocateInfo, descriptorSets.data()));
}

void Vkm::prepareData(const FilterProblem &filterProblem, const cv::Mat targetImage) {
  int depthBufferSize;
  int probBufferSize;
  if(targetImage.empty()){
    depthBufferSize = filterProblem.ref_depth.total()*filterProblem.ref_depth.elemSize();
    probBufferSize = filterProblem.ref_prob.total()*filterProblem.ref_prob.elemSize();
  } else {
    depthBufferSize = targetImage.total()*targetImage.elemSize();
    probBufferSize = targetImage.total()*targetImage.elemSize();
  }
  std::vector<VkWriteDescriptorSet> writeDescriptorSets;

  uint32_t set_id = 0;
  uint32_t bind_id = 0;
  paramsBuffer = std::make_unique<Vkb>(pvkd, sizeof(ParamsUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
  ParamsUBO paramsUbo;
  paramsUbo.ref_world_pose = getInversePose( filterProblem.ref_K, filterProblem.ref_pose);
  for(auto i = 0; i < filterProblem.src_rgbs.size(); i++){
    paramsUbo.src_world_pose[i] = getInversePose( filterProblem.src_Ks[i], filterProblem.src_poses[i]);
    paramsUbo.ref_src_pose[i] = getRelativePose(filterProblem.src_Ks[i], filterProblem.src_poses[i], filterProblem.ref_K, filterProblem.ref_pose);
    paramsUbo.src_ref_pose[i] = getRelativePose(filterProblem.ref_K, filterProblem.ref_pose, filterProblem.src_Ks[i], filterProblem.src_poses[i]);
  }
  paramsUbo.ref_size.x() = filterProblem.ref_rgb.cols;
  paramsUbo.ref_size.y() = filterProblem.ref_rgb.rows;
  paramsUbo.dst_size.x() = 2*filterProblem.ref_rgb.cols;
  paramsUbo.dst_size.y() = 2*filterProblem.ref_rgb.rows;
  paramsUbo.src_num = filterProblem.src_rgbs.size();
  paramsBuffer->uploadData(&paramsUbo, 0, sizeof(paramsUbo));
  auto & params = writeDescriptorSets.emplace_back();
  params.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  params.pNext = nullptr;
  params.dstSet = descriptorSets[set_id];
  params.dstBinding = descSetInfos[set_id][bind_id].binding;
  params.dstArrayElement = 0;
  params.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  params.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  VkDescriptorBufferInfo paramsBufferInfo;
  paramsBufferInfo.buffer = paramsBuffer->deviceBuffer;
  paramsBufferInfo.offset = 0;
  paramsBufferInfo.range = paramsBuffer->deviceSize;
  params.pImageInfo = nullptr;
  params.pBufferInfo = &paramsBufferInfo;
  params.pTexelBufferView = nullptr;

  bind_id = 1;
  depthBuffer = std::make_unique<Vkb>(pvkd, depthBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
  auto & depth = writeDescriptorSets.emplace_back();
  depth.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  depth.pNext = nullptr;
  depth.dstSet = descriptorSets[set_id];
  depth.dstBinding = descSetInfos[set_id][bind_id].binding;
  depth.dstArrayElement = 0;
  depth.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  depth.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  VkDescriptorBufferInfo depthBufferInfo;
  depthBufferInfo.buffer = depthBuffer->deviceBuffer;
  depthBufferInfo.offset = 0;
  depthBufferInfo.range = depthBuffer->deviceSize;
  depth.pImageInfo = nullptr;
  depth.pBufferInfo = &depthBufferInfo;
  depth.pTexelBufferView = nullptr;

  bind_id = 2;

  probBuffer = std::make_unique<Vkb>(pvkd, probBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
  auto & prob = writeDescriptorSets.emplace_back();
  prob.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  prob.pNext = nullptr;
  prob.dstSet = descriptorSets[set_id];
  prob.dstBinding = descSetInfos[set_id][bind_id].binding;
  prob.dstArrayElement = 0;
  prob.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  prob.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  VkDescriptorBufferInfo probBufferInfo;
  probBufferInfo.buffer = probBuffer->deviceBuffer;
  probBufferInfo.offset = 0;
  probBufferInfo.range = probBuffer->deviceSize;
  prob.pImageInfo = nullptr;
  prob.pBufferInfo = &probBufferInfo;
  prob.pTexelBufferView = nullptr;

  bind_id = 3;
  visibleViewsBuffer = std::make_unique<Vkb>(pvkd, probBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true);
  auto & score = writeDescriptorSets.emplace_back();
  score.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  score.pNext = nullptr;
  score.dstSet = descriptorSets[set_id];
  score.dstBinding = descSetInfos[set_id][bind_id].binding;
  score.dstArrayElement = 0;
  score.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  score.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  VkDescriptorBufferInfo scoreBufferInfo;
  scoreBufferInfo.buffer = visibleViewsBuffer->deviceBuffer;
  scoreBufferInfo.offset = 0;
  scoreBufferInfo.range = visibleViewsBuffer->deviceSize;
  score.pImageInfo = nullptr;
  score.pBufferInfo = &scoreBufferInfo;
  score.pTexelBufferView = nullptr;

  bind_id = 4;
  refRGBTexture = std::make_unique<Vktexture>(pvkd, filterProblem.ref_rgb.cols, filterProblem.ref_rgb.rows, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, true);
  if(filterProblem.ref_rgb.type() == CV_8UC3){
    cv::Mat rgba;
    cv::cvtColor(filterProblem.ref_rgb, rgba, cv::COLOR_BGR2BGRA);
    refRGBTexture->uploadData(rgba.data, 0, rgba.total()*rgba.elemSize());
  } else {
    CHECK(filterProblem.ref_rgb.type() == CV_8UC4);
    refRGBTexture->uploadData(filterProblem.ref_rgb.data, 0, filterProblem.ref_rgb.total()*filterProblem.ref_rgb.elemSize());
  }
  auto & ref_rgb = writeDescriptorSets.emplace_back();
  ref_rgb.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  ref_rgb.pNext = nullptr;
  ref_rgb.dstSet = descriptorSets[set_id];
  ref_rgb.dstBinding = descSetInfos[set_id][bind_id].binding;
  ref_rgb.dstArrayElement = 0;
  ref_rgb.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  ref_rgb.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  VkDescriptorImageInfo refRGBImageInfo;
  refRGBImageInfo.imageView = refRGBTexture->imageView;
  refRGBImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  refRGBImageInfo.sampler = refRGBImageInfo.sampler;
  ref_rgb.pImageInfo = &refRGBImageInfo;
  ref_rgb.pBufferInfo = nullptr;
  ref_rgb.pTexelBufferView = nullptr;

  bind_id = 5;
  refDepthProbTexture = std::make_unique<Vktexture>(pvkd, filterProblem.ref_depth.cols, filterProblem.ref_depth.rows, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, true);
  refDepthProbTexture->uploadDepthProb(filterProblem.ref_depth, filterProblem.ref_prob);
  auto & ref_depth_prob = writeDescriptorSets.emplace_back();
  ref_depth_prob.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  ref_depth_prob.pNext = nullptr;
  ref_depth_prob.dstSet = descriptorSets[set_id];
  ref_depth_prob.dstBinding = descSetInfos[set_id][bind_id].binding;
  ref_depth_prob.dstArrayElement = 0;
  ref_depth_prob.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  ref_depth_prob.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  VkDescriptorImageInfo refDepthProbImageInfo;
  refDepthProbImageInfo.imageView = refDepthProbTexture->imageView;
  refDepthProbImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  refDepthProbImageInfo.sampler = refDepthProbTexture->sampler;
  ref_depth_prob.pImageInfo = &refDepthProbImageInfo;
  ref_depth_prob.pBufferInfo = nullptr;
  ref_depth_prob.pTexelBufferView = nullptr;

  bind_id = 6;
  auto & src_rgbs = writeDescriptorSets.emplace_back();
  src_rgbs.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  src_rgbs.pNext = nullptr;
  src_rgbs.dstSet = descriptorSets[set_id];
  src_rgbs.dstBinding = descSetInfos[set_id][bind_id].binding;
  src_rgbs.dstArrayElement = 0;
  src_rgbs.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  src_rgbs.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  srcRGBTextures.clear();
  std::vector<VkDescriptorImageInfo> srcRGBImageInfos;
  for(auto i = 0; i < filterProblem.src_rgbs.size(); i++){
    const auto & srcRGBTexture = srcRGBTextures.emplace_back(new Vktexture(pvkd, filterProblem.src_rgbs[i].cols, filterProblem.src_rgbs[i].rows, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, true));
    if(filterProblem.src_rgbs[i].type() == CV_8UC3){
      cv::Mat rgba;
      cv::cvtColor(filterProblem.src_rgbs[i], rgba, cv::COLOR_BGR2BGRA);
      srcRGBTexture->uploadData(rgba.data, 0, rgba.total()*rgba.elemSize());
    } else {
      srcRGBTexture->uploadData(filterProblem.src_rgbs[i].data, 0, filterProblem.src_rgbs[i].total()*filterProblem.src_rgbs[i].elemSize());
    }

    auto &srcRGBImageInfo = srcRGBImageInfos.emplace_back();
    srcRGBImageInfo.imageView = srcRGBTexture->imageView;
    srcRGBImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    srcRGBImageInfo.sampler = srcRGBTexture->sampler;
  }
  CHECK(srcRGBImageInfos.size() == src_rgbs.descriptorCount);
  src_rgbs.pImageInfo = srcRGBImageInfos.data();
  src_rgbs.pBufferInfo = nullptr;
  src_rgbs.pTexelBufferView = nullptr;

  bind_id = 7;
  auto & src_depth_probs = writeDescriptorSets.emplace_back();
  src_depth_probs.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  src_depth_probs.pNext = nullptr;
  src_depth_probs.dstSet = descriptorSets[set_id];
  src_depth_probs.dstBinding = descSetInfos[set_id][bind_id].binding;
  src_depth_probs.dstArrayElement = 0;
  src_depth_probs.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  src_depth_probs.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  srcDepthProbTextures.clear();
  std::vector<VkDescriptorImageInfo> srcDepthProbImageInfos;
  for(auto i = 0; i < filterProblem.src_depths.size(); i++){
    const auto & srcDepthProbTexture = srcDepthProbTextures.emplace_back(new Vktexture(pvkd, filterProblem.src_depths[i].cols, filterProblem.src_depths[i].rows, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, true));
    srcDepthProbTexture->uploadDepthProb(filterProblem.src_depths[i], filterProblem.src_probs[i]);

    auto &srcDepthProbImageInfo = srcDepthProbImageInfos.emplace_back();
    srcDepthProbImageInfo.imageView = srcDepthProbTexture->imageView;
    srcDepthProbImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    srcDepthProbImageInfo.sampler = srcDepthProbTexture->sampler;
  }
  CHECK(srcDepthProbImageInfos.size() == src_depth_probs.descriptorCount);
  src_depth_probs.pImageInfo = srcDepthProbImageInfos.data();
  src_depth_probs.pBufferInfo = nullptr;
  src_depth_probs.pTexelBufferView = nullptr;

  bind_id = 8;
  auto & proj_rgbs = writeDescriptorSets.emplace_back();
  proj_rgbs.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  proj_rgbs.pNext = nullptr;
  proj_rgbs.dstSet = descriptorSets[set_id];
  proj_rgbs.dstBinding = descSetInfos[set_id][bind_id].binding;
  proj_rgbs.dstArrayElement = 0;
  proj_rgbs.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  proj_rgbs.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  VkDescriptorImageInfo projRGBImageInfo;
  projRGBImageInfo.imageView = colorAttachment->imageView;
  projRGBImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  projRGBImageInfo.sampler = colorAttachment->sampler;
  proj_rgbs.pImageInfo = &projRGBImageInfo;
  proj_rgbs.pBufferInfo = nullptr;
  proj_rgbs.pTexelBufferView = nullptr;

  bind_id = 9;
  auto & proj_depth_probs = writeDescriptorSets.emplace_back();
  proj_depth_probs.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  proj_depth_probs.pNext = nullptr;
  proj_depth_probs.dstSet = descriptorSets[set_id];
  proj_depth_probs.dstBinding = descSetInfos[set_id][bind_id].binding;
  proj_depth_probs.dstArrayElement = 0;
  proj_depth_probs.descriptorCount = descSetInfos[set_id][bind_id].descriptorCount;
  proj_depth_probs.descriptorType = descSetInfos[set_id][bind_id].descriptorType;
  VkDescriptorImageInfo projDepthProbImageInfo;
  projDepthProbImageInfo.imageView = infoAttachment->imageView;
  projDepthProbImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
  projDepthProbImageInfo.sampler = colorAttachment->sampler;
  proj_depth_probs.pImageInfo = &projDepthProbImageInfo;
  proj_depth_probs.pBufferInfo = nullptr;
  proj_depth_probs.pTexelBufferView = nullptr;
  vkUpdateDescriptorSets(pvkd->device, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
}