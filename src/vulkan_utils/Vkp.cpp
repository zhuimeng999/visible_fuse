//
// Created by lucius on 8/5/20.
//

#include "vulkan_utils/Vkd.h"
#include "vulkan_utils/Vkm.h"
#include "vulkan_utils/Vkp.h"
#include "log/log.h"
#include <fstream>

Vkp::Vkp(std::shared_ptr<Vkd> &pVkd, std::shared_ptr<Vkm> &pVkm) : pvkd(pVkd), pvkm(pVkm),
                                                                   pipeLineCache(VK_NULL_HANDLE),
                                                                   pipelineLayout(VK_NULL_HANDLE),
                                                                   pipeline(VK_NULL_HANDLE){
  VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  pipelineCacheCreateInfo.pNext = nullptr;
  pipelineCacheCreateInfo.pInitialData = nullptr;
  pipelineCacheCreateInfo.initialDataSize = 0;
  pipelineCacheCreateInfo.flags = 0;
  CHECK_VULKAN(vkCreatePipelineCache(pvkd->device, &pipelineCacheCreateInfo, nullptr, &pipeLineCache));

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
  pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.pNext = nullptr;
  pipelineLayoutCreateInfo.pSetLayouts = pvkm->descriptorSetLayouts.data();
  pipelineLayoutCreateInfo.setLayoutCount = pvkm->descriptorSetLayouts.size();
  pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
  pipelineLayoutCreateInfo.flags = 0;
  CHECK_VULKAN(vkCreatePipelineLayout(pvkd->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}

Vkp::~Vkp() {
  vkDestroyPipelineLayout(pvkd->device, pipelineLayout, nullptr);
  vkDestroyPipelineCache(pvkd->device, pipeLineCache, nullptr);
}

VkShaderModule Vkp::loadShader(const std::string &filename) {
  std::ifstream in(filename, std::ios::ate | std::ios::binary);
  CHECK(in.is_open())
  size_t fileSize = (size_t) in.tellg();
  std::vector<char> shader_code(fileSize);
  in.seekg(0);
  in.read(shader_code.data(), fileSize);
  in.close();

  VkShaderModuleCreateInfo createInfo;
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.pNext = nullptr;
  createInfo.codeSize = shader_code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(shader_code.data());
  createInfo.flags = 0;
  VkShaderModule shaderModule;
  CHECK_VULKAN(vkCreateShaderModule(pvkd->device, &createInfo, nullptr, &shaderModule));
  return shaderModule;
}
