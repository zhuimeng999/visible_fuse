//
// Created by lucius on 8/2/20.
//

#ifndef UNTITLED_VKPG_H
#define UNTITLED_VKPG_H

#include "vulkan_utils/Vkp.h"
#include <Eigen/Eigen>

class Vkpg : public Vkp {
public:
  explicit Vkpg(std::shared_ptr<Vkd> &pVkd, std::shared_ptr<Vkm> &pVkr);
  ~Vkpg();

  void createPipeline();
  void createRenderPass();
  void createFrameBuffer();

public:
  VkRenderPass renderPass;
  VkFramebuffer framebuffer;
};


#endif //UNTITLED_VKPG_H
