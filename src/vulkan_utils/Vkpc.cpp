//
// Created by lucius on 8/1/20.
//

#include "vulkan_utils/Vkd.h"
#include "vulkan_utils/Vkm.h"
#include "vulkan_utils/Vkpc.h"
#include "log/log.h"
#include <fstream>

Vkpc::Vkpc(std::shared_ptr<Vkd> &pVkd, std::shared_ptr<Vkm> &pVkr, const std::string & shaderPath) : Vkp(pVkd, pVkr){
  auto computeShaderModule = loadShader(shaderPath);
  VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
  pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipelineShaderStageCreateInfo.pNext = nullptr;
  pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipelineShaderStageCreateInfo.module = computeShaderModule;
  pipelineShaderStageCreateInfo.pName = "main";
  pipelineShaderStageCreateInfo.pSpecializationInfo = nullptr;
  pipelineShaderStageCreateInfo.flags = 0;

  VkComputePipelineCreateInfo computePipelineCreateInfo;
  computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.pNext = nullptr;
  computePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  computePipelineCreateInfo.basePipelineIndex = 0;
  computePipelineCreateInfo.layout = pipelineLayout;
  computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
  computePipelineCreateInfo.flags = 0;
  CHECK_VULKAN(vkCreateComputePipelines(pvkd->device, pipeLineCache, 1, &computePipelineCreateInfo, nullptr, &pipeline));

  vkDestroyShaderModule(pvkd->device, computeShaderModule, nullptr);
}

Vkpc::~Vkpc() {
  vkDestroyPipeline(pvkd->device, pipeline, nullptr);
}

