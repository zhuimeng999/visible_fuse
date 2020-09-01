//
// Created by lucius on 8/2/20.
//

#include "vulkan_utils/Vkd.h"
#include "vulkan_utils/Vkm.h"
#include "vulkan_utils/Vktexture.h"
#include "vulkan_utils/Vkpg.h"
#include "log/log.h"
#include <fstream>

Vkpg::Vkpg(std::shared_ptr<Vkd> &pVkd, std::shared_ptr<Vkm> &pVkr) : Vkp(pVkd, pVkr),
                                                                     renderPass(VK_NULL_HANDLE){
  createRenderPass();
  createFrameBuffer();
  createPipeline();
}

Vkpg::~Vkpg() {
  vkDestroyPipeline(pvkd->device, pipeline, nullptr);
  vkDestroyFramebuffer(pvkd->device, framebuffer, nullptr);
  vkDestroyRenderPass(pvkd->device, renderPass, nullptr);
}

void Vkpg::createPipeline() {
  auto vertexShader = loadShader("glsl/shader.vert.spv");
  auto geometryShader = loadShader("glsl/shader.geom.spv");
  auto fragmentShader = loadShader("glsl/shader.frag.spv");

  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
  auto &vertexShaderStageCreateInfo = shaderStages.emplace_back();
  vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexShaderStageCreateInfo.pNext = nullptr;
  vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderStageCreateInfo.pSpecializationInfo = nullptr;
  vertexShaderStageCreateInfo.module = vertexShader;
  vertexShaderStageCreateInfo.pName = "main";
  vertexShaderStageCreateInfo.flags = 0;

  auto &geometryShaderStageCreateInfo = shaderStages.emplace_back();
  geometryShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  geometryShaderStageCreateInfo.pNext = nullptr;
  geometryShaderStageCreateInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
  geometryShaderStageCreateInfo.pSpecializationInfo = nullptr;
  geometryShaderStageCreateInfo.module = geometryShader;
  geometryShaderStageCreateInfo.pName = "main";
  geometryShaderStageCreateInfo.flags = 0;

  auto &fragmentShaderStageCreateInfo = shaderStages.emplace_back();
  fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragmentShaderStageCreateInfo.pNext = nullptr;
  fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderStageCreateInfo.pSpecializationInfo = nullptr;
  fragmentShaderStageCreateInfo.module = fragmentShader;
  fragmentShaderStageCreateInfo.pName = "main";
  fragmentShaderStageCreateInfo.flags = 0;

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();
  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
  pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  pipelineVertexInputStateCreateInfo.pNext = nullptr;
  pipelineVertexInputStateCreateInfo.flags = 0;
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = bindingDescription.data();
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 0;
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;


  // Create pipeline
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
  inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyState.pNext = nullptr;
  inputAssemblyState.flags = 0;
  inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyState.primitiveRestartEnable = VK_FALSE;

  VkPipelineRasterizationStateCreateInfo rasterizationState{};
  rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationState.pNext = nullptr;
  rasterizationState.flags = 0;
  rasterizationState.depthClampEnable = VK_FALSE;
  rasterizationState.rasterizerDiscardEnable = VK_FALSE;
  rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizationState.depthBiasEnable = VK_FALSE;
  rasterizationState.depthBiasConstantFactor = 0;
  rasterizationState.depthBiasClamp = 0;
  rasterizationState.depthBiasSlopeFactor = 0;
  rasterizationState.lineWidth = 1.0f;

  std::vector<VkPipelineColorBlendAttachmentState> pipelineColorBlendAttachmentStates = {
          {.blendEnable = VK_FALSE, .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT},
          {.blendEnable = VK_FALSE, .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}
  };

  VkPipelineColorBlendStateCreateInfo colorBlendState;
  colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendState.pNext = nullptr;
  colorBlendState.flags = 0;
  colorBlendState.blendConstants[0] = 0;
  colorBlendState.blendConstants[1] = 0;
  colorBlendState.blendConstants[2] = 0;
  colorBlendState.blendConstants[3] = 0;
  colorBlendState.pAttachments = pipelineColorBlendAttachmentStates.data();
  colorBlendState.attachmentCount = pipelineColorBlendAttachmentStates.size();
  colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
  colorBlendState.logicOpEnable = VK_FALSE;

  VkPipelineDepthStencilStateCreateInfo depthStencilState{};
  depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilState.depthTestEnable = VK_FALSE;
  depthStencilState.depthWriteEnable = VK_FALSE;
  depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
  depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;

  VkPipelineViewportStateCreateInfo viewportState;
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.pNext = nullptr;
  viewportState.flags = 0;
  viewportState.pViewports = nullptr;
  viewportState.viewportCount = 1;
  viewportState.pScissors = nullptr;
  viewportState.scissorCount = 1;

  VkPipelineMultisampleStateCreateInfo multisampleState;
  multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleState.pNext = nullptr;
  multisampleState.flags = 0;
  multisampleState.alphaToCoverageEnable = VK_FALSE;
  multisampleState.alphaToOneEnable = VK_FALSE;
  multisampleState.sampleShadingEnable = VK_FALSE;
  multisampleState.minSampleShading = VK_NULL_HANDLE;
  multisampleState.pSampleMask = nullptr;
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  std::array<VkDynamicState, 2> dynamicStateEnables = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
  };
  VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
  pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
  pipelineDynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
  pipelineDynamicStateCreateInfo.flags = 0;

  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
  graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphicsPipelineCreateInfo.pNext = nullptr;
  graphicsPipelineCreateInfo.flags = 0;
  graphicsPipelineCreateInfo.pStages = shaderStages.data();
  graphicsPipelineCreateInfo.stageCount = shaderStages.size();
  graphicsPipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
  graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  graphicsPipelineCreateInfo.pTessellationState = nullptr;
  graphicsPipelineCreateInfo.pViewportState = &viewportState;
  graphicsPipelineCreateInfo.pRasterizationState = &rasterizationState;
  graphicsPipelineCreateInfo.pMultisampleState = &multisampleState;
  graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;
  graphicsPipelineCreateInfo.pColorBlendState = &colorBlendState;
  graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
  graphicsPipelineCreateInfo.layout = pipelineLayout;
  graphicsPipelineCreateInfo.renderPass = renderPass;
  graphicsPipelineCreateInfo.subpass = VK_NULL_HANDLE;
  graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineCreateInfo.basePipelineIndex = 0;

  CHECK_VULKAN(vkCreateGraphicsPipelines(pvkd->device, pipeLineCache, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline));

  vkDestroyShaderModule(pvkd->device, vertexShader, nullptr);
  vkDestroyShaderModule(pvkd->device, geometryShader, nullptr);
  vkDestroyShaderModule(pvkd->device, fragmentShader, nullptr);
}

void Vkpg::createRenderPass() {
  std::array<VkAttachmentDescription, 3> attchmentDescriptions = {};
  // Color attachment
  attchmentDescriptions[0].format = pvkm->colorAttachment->format;
  attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

  // info
  attchmentDescriptions[1].format = pvkm->infoAttachment->format;
  attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

  // Depth attachment
  attchmentDescriptions[2].format = pvkm->depthAttachment->format;
  attchmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
  attchmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attchmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attchmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attchmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attchmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

  std::vector<VkAttachmentReference> colorReference = {{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
                                                       { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }};
  VkAttachmentReference depthReference = { 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = colorReference.size();
  subpassDescription.pColorAttachments = colorReference.data();
  subpassDescription.pDepthStencilAttachment = &depthReference;

  // Use subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies;

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  /*
  Bit mask that specifies which view rendering is broadcast to
  0011 = Broadcast to first and second view (layer)
*/
  uint32_t viewMask = 0;

  /*
    Bit mask that specifices correlation between views
    An implementation may use this for optimizations (concurrent render)
  */
  uint32_t correlationMask = 0;
  for(auto i = 0; i < pvkm->colorAttachment->arrayLayers; i++){
    viewMask |= (1u << i);
    correlationMask |= (1u << i);
  }

  VkRenderPassMultiviewCreateInfo renderPassMultiviewCI{};
  renderPassMultiviewCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
  renderPassMultiviewCI.subpassCount = 1;
  renderPassMultiviewCI.pViewMasks = &viewMask;
  renderPassMultiviewCI.correlationMaskCount = 1;
  renderPassMultiviewCI.pCorrelationMasks = &correlationMask;

  // Create the actual renderpass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.pNext = &renderPassMultiviewCI;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
  renderPassInfo.pAttachments = attchmentDescriptions.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();
  CHECK_VULKAN(vkCreateRenderPass(pvkd->device, &renderPassInfo, nullptr, &renderPass));
}

void Vkpg::createFrameBuffer() {
  std::array<VkImageView, 3> attachments = {
          pvkm->colorAttachment->imageView,
          pvkm->infoAttachment->imageView,
          pvkm->depthAttachment->imageView
  };

  VkFramebufferCreateInfo framebufferCreateInfo{};
  framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferCreateInfo.renderPass = renderPass;
  framebufferCreateInfo.attachmentCount = attachments.size();
  framebufferCreateInfo.pAttachments = attachments.data();
  framebufferCreateInfo.width = pvkm->colorAttachment->width;
  framebufferCreateInfo.height = pvkm->colorAttachment->height;
  framebufferCreateInfo.layers = 1;
  CHECK_VULKAN(vkCreateFramebuffer(pvkd->device, &framebufferCreateInfo, nullptr, &framebuffer));
}
