//
// Created by lucius on 8/5/20.
//

#ifndef UNTITLED_VKP_H
#define UNTITLED_VKP_H

#include <vulkan/vulkan.hpp>

class Vkd;
class Vkm;

class Vkp {
public:
  explicit Vkp(std::shared_ptr<Vkd> &pVkd, std::shared_ptr<Vkm> &pVkm);
  ~Vkp();

  VkPipelineCache pipeLineCache;
  VkPipelineLayout pipelineLayout;
  VkPipeline pipeline;

protected:
  std::shared_ptr<Vkd> pvkd;
  std::shared_ptr<Vkm> pvkm;

  VkShaderModule loadShader(const std::string &filename);
};


#endif //UNTITLED_VKP_H
