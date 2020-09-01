//
// Created by lucius on 8/1/20.
//

#ifndef UNTITLED_VKM_H
#define UNTITLED_VKM_H

#include <vulkan/vulkan.hpp>
#include <Eigen/Eigen>
#include "utils/FilterProblem.h"

class Vkd;
class Vkb;
class Vkc;
class Vktexture;

struct FrameBufferAttachment {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView view;
};

struct Vertex {
  float pos[3];
  float uv[2];

  static std::array<VkVertexInputBindingDescription, 1> getBindingDescription() {
    std::array<VkVertexInputBindingDescription, 1> bindingDescriptions;
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = sizeof(Vertex);
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescriptions;
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, uv);

    return attributeDescriptions;
  }
};

struct ParamsUBO {
  static_assert(sizeof(Eigen::Matrix4f) == 16*4, "format error");
  static_assert(sizeof(Eigen::Vector2f) == 2*4, "format error");
  Eigen::Matrix4f ref_world_pose;
  Eigen::Matrix4f src_world_pose[10];
  Eigen::Matrix4f ref_src_pose[10];
  Eigen::Matrix4f src_ref_pose[10];
  Eigen::Vector2f ref_size;
  Eigen::Vector2f dst_size;
  uint32_t src_num;
};


class Vkm {
//  static_assert(sizeof(ParamsUBO) == (16*10+2+1)*4, "format error");
public:
  explicit Vkm(std::shared_ptr<Vkd> &pVkd, VkDeviceSize maxWidth, VkDeviceSize maxHeight);
  ~Vkm();

  void prepareData(const FilterProblem &filterProblem, const cv::Mat targetImage = cv::Mat());

  void buildDescriptorSetInfos();
  void buildDescriptorSets();
  std::vector<std::vector<VkDescriptorSetLayoutBinding>> descSetInfos;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
  std::vector<VkDescriptorSet> descriptorSets;

  std::unique_ptr<Vkb> paramsBuffer;
  std::unique_ptr<Vkb> depthBuffer;
  std::unique_ptr<Vkb> probBuffer;
  std::unique_ptr<Vkb> visibleViewsBuffer;
  std::unique_ptr<Vktexture> refRGBTexture;
  std::unique_ptr<Vktexture> refDepthProbTexture;
  std::vector<std::unique_ptr<Vktexture>> srcRGBTextures;
  std::vector<std::unique_ptr<Vktexture>> srcDepthProbTextures;

  std::unique_ptr<Vktexture> colorAttachment;
  std::unique_ptr<Vktexture> infoAttachment;
  std::unique_ptr<Vktexture> depthAttachment;

private:
  std::shared_ptr<Vkd> pvkd;
};


#endif //UNTITLED_VKM_H
