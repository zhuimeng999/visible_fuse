//
// Created by lucius on 8/13/20.
//

#ifndef VISIBLE_FUSE_FILTERPROBLEM_H
#define VISIBLE_FUSE_FILTERPROBLEM_H

#include <opencv2/core.hpp>
#include <Eigen/Eigen>

class FilterProblem {
public:
  cv::Mat ref_rgb;
  cv::Mat ref_depth;
  cv::Mat ref_prob;
  cv::Mat visible_view;
  Eigen::Isometry3d ref_pose;
  Eigen::Matrix3d ref_K;
  std::vector<cv::Mat> src_rgbs;
  std::vector<cv::Mat> src_depths;
  std::vector<cv::Mat> src_probs;
  std::vector<Eigen::Isometry3d> src_poses;
  std::vector<Eigen::Matrix3d> src_Ks;
};

class FilterResult {
public:
  cv::Mat rgb;
  cv::Mat d;
  cv::Mat p;
  cv::Mat v;
};


#endif //VISIBLE_FUSE_FILTERPROBLEM_H
