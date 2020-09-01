//
// Created by lucius on 8/11/20.
//

#include "utils/DepthMapFilter.h"
#include "vulkan_utils/Vki.h"
#include "vulkan_utils/Vkd.h"
#include "vulkan_utils/Vkm.h"
#include "vulkan_utils/Vkpc.h"
#include "vulkan_utils/Vkpg.h"
#include "vulkan_utils/Vkc.h"
#include "vulkan_utils/Vkb.h"
#include "vulkan_utils/Vktexture.h"
#include "log/log.h"
#include "utils/DebugDisplay.h"
#include <opencv2/imgproc.hpp>

DepthMapFilter::DepthMapFilter(uint32_t maxWidth, uint32_t maxHeight) : pvki(new Vki(false)),
                                   pvkd(new Vkd(pvki)),
                                   pvkc(new Vkc(pvkd)),
                                   pvkm(new Vkm(pvkd, maxWidth, maxHeight)),
                                   pvkpc(new Vkpc(pvkd, pvkm, "glsl/shader.comp.spv")),
                                   LRCPipeline(new Vkpc(pvkd, pvkm, "glsl/left_right_check.comp.spv")),
                                   holeFillPipeline(new Vkpc(pvkd, pvkm, "glsl/hole_fill.comp.spv")),
                                   pvkpg(new Vkpg(pvkd, pvkm)) {
}

void DepthMapFilter::probConsistantFilter(const FilterProblem &filterProblem, FilterResult &filterResult) {
  filterResult.d.create(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, filterProblem.ref_depth.type());
  filterResult.p.create(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, filterProblem.ref_prob.type());
  filterResult.v.create(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, CV_32SC1);
  cv::Mat ia(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, CV_32FC4);
  cv::Mat out_rgb(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, CV_8UC4);
  pvkm->prepareData(filterProblem);
  pvkc->beginCommandBuffer();
  pvkc->resourcePrepare(pvkm);
  pvkc->addComputePipeline(LRCPipeline, pvkm);
//  pvkc->buildCommandBuffer(pvkpc, pvkpg, pvkm);
  pvkc->resourceDownload(pvkm);
  pvkc->endCommandBuffer();
  pvkc->sumit();
  pvkc->waitFinished();
  pvkm->depthBuffer->downloadData(filterResult.d.data, 0, filterResult.d.total() * filterResult.d.elemSize());
  pvkm->probBuffer->downloadData(filterResult.p.data, 0, filterResult.p.total() * filterResult.p.elemSize());
  pvkm->visibleViewsBuffer->downloadData(filterResult.v.data, 0, filterResult.v.total() * filterResult.v.elemSize());

  filterResult.rgb = filterProblem.ref_rgb;
}

void DepthMapFilter::refineDepthMap(const FilterProblem &filterProblem, FilterResult &filterResult) {
  filterResult.d.create(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, filterProblem.ref_depth.type());
  filterResult.p.create(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, filterProblem.ref_prob.type());
  cv::Mat score(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, CV_32SC1);
  cv::Mat ia(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, CV_32FC4);
  cv::Mat out_rgb(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, CV_8UC4);
  pvkm->prepareData(filterProblem);
  pvkc->beginCommandBuffer();
  pvkc->resourcePrepare(pvkm);
  pvkc->buildCommandBuffer(pvkpc, pvkpg, pvkm);
  pvkc->resourceDownload(pvkm);
  pvkc->endCommandBuffer();
  pvkc->sumit();
  pvkc->waitFinished();
  pvkm->depthBuffer->downloadData(filterResult.d.data, 0, filterResult.d.total() * filterResult.d.elemSize());
  pvkm->probBuffer->downloadData(filterResult.p.data, 0, filterResult.p.total() * filterResult.p.elemSize());
  pvkm->visibleViewsBuffer->downloadData(score.data, 0, score.total() * score.elemSize());

  pvkm->colorAttachment->downloadData(out_rgb.data, out_rgb.total()*out_rgb.elemSize(), out_rgb.total()*out_rgb.elemSize());
  pvkm->infoAttachment->downloadData(ia.data, 0, ia.total()*ia.elemSize());

  filterResult.rgb = filterProblem.ref_rgb;
}

void DepthMapFilter::meanFilter(const FilterProblem &filterProblem1, FilterResult &filterResult) {
  FilterProblem filterProblem(filterProblem1);
  filterProblem.src_depths[0] = filterProblem.ref_depth;
  filterProblem.src_probs[0] = filterProblem.ref_prob;
  filterProblem.src_rgbs[0] = filterProblem.ref_rgb;
  filterResult.d.create(filterProblem.ref_rgb.rows * 2, filterProblem.ref_rgb.cols * 2, filterProblem.ref_depth.type());
  filterResult.p.create(filterProblem.ref_rgb.rows * 2, filterProblem.ref_rgb.cols * 2, filterProblem.ref_prob.type());
  cv::Mat visible_views(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, CV_32SC1);

  cv::Mat out_rgb(filterProblem.ref_rgb.rows, filterProblem.ref_rgb.cols, CV_8UC4);
  pvkm->prepareData(filterProblem, filterResult.d);
  pvkm->visibleViewsBuffer->uploadData(filterProblem.visible_view.data, 0, filterProblem.visible_view.total() * filterProblem.visible_view.elemSize());
  pvkc->beginCommandBuffer();
  pvkc->resourcePrepare(pvkm);
  pvkc->uploadBuffer(pvkm->visibleViewsBuffer);
  pvkc->addComputePipeline(holeFillPipeline, pvkm, 2);
  pvkc->resourceDownload(pvkm);
  pvkc->endCommandBuffer();
  pvkc->sumit();
  pvkc->waitFinished();
  pvkm->depthBuffer->downloadData(filterResult.d.data, 0, filterResult.d.total() * filterResult.d.elemSize());
  pvkm->probBuffer->downloadData(filterResult.p.data, 0, filterResult.p.total() * filterResult.p.elemSize());
//  pvkm->visibleViewsBuffer->downloadData(visible_views.data, 0, visible_views.total() * visible_views.elemSize());

  cv::resize(filterProblem.ref_rgb, filterResult.rgb, filterResult.d.size());
}
