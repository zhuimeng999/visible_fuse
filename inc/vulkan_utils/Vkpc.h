//
// Created by lucius on 8/1/20.
//

#ifndef UNTITLED_VKPC_H
#define UNTITLED_VKPC_H


#include "vulkan_utils/Vkp.h"

class Vkpc : public Vkp {
public:
  Vkpc(std::shared_ptr<Vkd> &pVkd, std::shared_ptr<Vkm> &pvkr, const std::string & shaderPath);
  ~Vkpc();
};


#endif //UNTITLED_VKPC_H
