//
// Created by lucius on 8/11/20.
//

#ifndef VISIBLE_FUSE_DEPTHMAPFILTER_H
#define VISIBLE_FUSE_DEPTHMAPFILTER_H

#include <vulkan/vulkan.hpp>
#include "utils/FilterProblem.h"

class Vki;
class Vkd;
class Vkc;
class Vkm;
class Vkpc;
class Vkpg;

class DepthMapFilter {
public:
  explicit DepthMapFilter(uint32_t maxWidth, uint32_t maxHeight);

  void probConsistantFilter(const FilterProblem &filterProblem, FilterResult &filterResult);
  void refineDepthMap(const FilterProblem &filterProblem, FilterResult &filterResult);
  void meanFilter(const FilterProblem &filterProblem, FilterResult &filterResult);

  std::shared_ptr<Vki> pvki;
  std::shared_ptr<Vkd> pvkd;
  std::shared_ptr<Vkc> pvkc;
  std::shared_ptr<Vkm> pvkm;
  std::shared_ptr<Vkpc> pvkpc;
  std::shared_ptr<Vkpc> LRCPipeline;
  std::shared_ptr<Vkpc> holeFillPipeline;
  std::shared_ptr<Vkpg> pvkpg;
};


#endif //VISIBLE_FUSE_DEPTHMAPFILTER_H
