#include "utils/DepthMapFilter.h"
#include "log/log.h"
#include "utils/ply.h"
#include <OpenImageIO/imageio.h>
#include <boost/log/expressions.hpp>
#include <fstream>
#include <boost/filesystem.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace fs = boost::filesystem;

struct {
  uint32_t max_width;
  uint32_t max_height;
  uint32_t max_pair_num;
  std::vector<std::string> image_paths;
  std::vector<std::vector<uint32_t>> image_pairs;
  std::vector<Eigen::Isometry3d> image_poses;
  std::vector<Eigen::Matrix3d> image_Ks;
} project_info;

inline static Eigen::Matrix4f getRelativePose(const Eigen::Matrix3d &ref_K, const Eigen::Isometry3d &ref_pose, const Eigen::Matrix3d &src_K, const Eigen::Isometry3d &src_pose){
  const auto &&t1 = ref_pose*src_pose.inverse();
  Eigen::Matrix4d res = Eigen::Matrix4d::Identity();
  res.block(0, 0, 3, 3) = ref_K*t1.rotation()*src_K.inverse();
  res.block(0, 3, 3, 1) = ref_K*t1.translation();
//  BOOST_LOG_TRIVIAL(info) << res;
//  BOOST_LOG_TRIVIAL(info) << t1.matrix();
//  BOOST_LOG_TRIVIAL(info) << t2.matrix();
//  BOOST_LOG_TRIVIAL(info) << ref_K*t1.rotation();
//  BOOST_LOG_TRIVIAL(info) << ref_K*t1.translation();
//  BOOST_LOG_TRIVIAL(info) << t1.translation();
  return res.cast<float>();
}

void load_project(const std::string &proj_dir){
  std::ifstream in(proj_dir + "/image_lists.txt");
  std::string img_path;
  CHECK(in.is_open());
  while(true){
    if(in >> img_path){
      project_info.image_paths.push_back(img_path);
    }
    if(in.eof()){
      break;
    }
  }
  in.close();
  BOOST_LOG_TRIVIAL(info) << "total " << project_info.image_paths.size() << " images";
  uint32_t max_width = 0;
  uint32_t max_height = 0;
  for(const auto &it: project_info.image_paths){
    auto img = OIIO::ImageInput::open(it);
    CHECK(img);
    const auto &img_spec = img->spec();
    if(max_width < img_spec.width){
      max_width = img_spec.width;
    }
    if(max_height < img_spec.height){
      max_height = img_spec.height;
    }
    img->close();
  }
  project_info.max_width = max_width;
  project_info.max_height = max_height;
  BOOST_LOG_TRIVIAL(info) << "max image width: " << max_width << ", max image height: " << max_height;

  in.open(proj_dir + "/pair.txt");
  CHECK(in.is_open());
  uint32_t total_view_num = 0;
  in >> total_view_num;
  CHECK(total_view_num == project_info.image_paths.size());
  project_info.image_pairs.resize(total_view_num);
  uint32_t ref_id;
  uint32_t src_num;
  uint32_t src_id;
  float src_score;
  for(auto i = 0; i < total_view_num; i++){
    in >> ref_id >> src_num;
    CHECK(ref_id == i);
    project_info.image_pairs[ref_id].reserve(src_num);
    for(auto j = 0; j < src_num; j++){
      in >> src_id >> src_score;
      project_info.image_pairs[ref_id].push_back(src_id);
    }
  }
  std::string dummy;
  in >> dummy;
  CHECK(dummy == "");
  CHECK(in.eof());
  in.close();

  uint32_t max_pair_num = 0;
  for(const auto &it: project_info.image_pairs){
    if(max_pair_num < it.size()){
      max_pair_num = it.size();
    }
  }
  project_info.max_pair_num = max_pair_num;

  project_info.image_poses.resize(total_view_num);
  project_info.image_Ks.resize(total_view_num);
  for(auto i = 0; i < project_info.image_paths.size(); i++){
    Eigen::Matrix4d pose;
    in.open(fs::path(project_info.image_paths[i]).replace_extension("txt"));
    CHECK(in.is_open());
    std::string signature;
    in >> signature;
    for(int j = 0; j < 4; j++){
      for(int k = 0; k < 4; k++){
        in >> pose(j, k);
      }
    }
    project_info.image_poses[i] = pose;
    in >> signature;
    for(int j = 0; j < 3; j++){
      for(int k = 0; k < 3; k++){
        in >> project_info.image_Ks[i](j, k);
      }
    }
    float dummy;
    in >> dummy >> dummy >> dummy >> dummy >> dummy;
    CHECK(in.eof());
    in.close();
  }
}

#define DEPTH_PATH(img_name) img_name.substr(0, img_name.size() - 4) + "_init.pfm"
#define PROB_PATH(img_name) img_name.substr(0, img_name.size() - 4) + "_prob.pfm"


int main(int argc, char *argv[]) {
  boost::log::core::get()->set_filter (
                  boost::log::trivial::severity >= boost::log::trivial::info
          );
//  load_project("/home/lucius/data/mvs_training/test");
//  load_project("/home/lucius/data/tmp/depths_mvsnet");
  load_project("/home/lucius/data/mvs_colmap2rmvsnet/depths_mvsnet");
  std::string work_dir = "/home/lucius/data/tmp/work";
  std::vector<PlyPoint> fused_points;

  /* prob consistant filter*/
  BOOST_LOG_TRIVIAL(info) << "start prob consistant filter";
  std::vector<FilterResult> filterResults(project_info.image_paths.size());
  for(auto i = 0; i < project_info.image_pairs.size(); i++){
    BOOST_LOG_TRIVIAL(info) << "prob consistant filter " << i << " ... ";
    FilterProblem filterProblem;
    filterProblem.ref_pose = project_info.image_poses[i];
    filterProblem.ref_K = project_info.image_Ks[i];
    filterProblem.ref_rgb = cv::imread(project_info.image_paths[i], cv::IMREAD_UNCHANGED);
    filterProblem.ref_depth = cv::imread(DEPTH_PATH(project_info.image_paths[i]), cv::IMREAD_UNCHANGED);
    filterProblem.ref_prob = cv::imread(PROB_PATH(project_info.image_paths[i]), cv::IMREAD_UNCHANGED);

    for(const auto it: project_info.image_pairs[i]){
      filterProblem.src_poses.push_back(project_info.image_poses[it]);
      filterProblem.src_Ks.push_back(project_info.image_Ks[it]);
      filterProblem.src_rgbs.emplace_back(cv::imread(project_info.image_paths[it], cv::IMREAD_UNCHANGED));
      filterProblem.src_depths.emplace_back(cv::imread(DEPTH_PATH(project_info.image_paths[it]), cv::IMREAD_UNCHANGED));
      filterProblem.src_probs.emplace_back(cv::imread(PROB_PATH(project_info.image_paths[it]), cv::IMREAD_UNCHANGED));
    }
    DepthMapFilter dmf(project_info.max_width, project_info.max_height);
    auto & filterResult = filterResults[i];
    dmf.probConsistantFilter(filterProblem, filterResult);
  }

  /* visible filter */
  BOOST_LOG_TRIVIAL(info) << "start visible consistant filter";
  std::vector<FilterResult> refineResults(project_info.image_paths.size());
  for(auto i = 0; i < project_info.image_pairs.size(); i++){
    BOOST_LOG_TRIVIAL(info) << "visible filter " << i << " ... ";
    FilterProblem filterProblem;
    filterProblem.ref_pose = project_info.image_poses[i];
    filterProblem.ref_K = project_info.image_Ks[i];
    filterProblem.ref_rgb = filterResults[i].rgb;
    filterProblem.ref_depth = filterResults[i].d;
    filterProblem.ref_prob = filterResults[i].p;

    for(const auto it: project_info.image_pairs[i]){
      filterProblem.src_poses.push_back(project_info.image_poses[it]);
      filterProblem.src_Ks.push_back(project_info.image_Ks[it]);
      filterProblem.src_rgbs.emplace_back(filterResults[it].rgb);
      filterProblem.src_depths.emplace_back(filterResults[it].d);
      filterProblem.src_probs.emplace_back(filterResults[it].p);
    }
    DepthMapFilter dmf(project_info.max_width, project_info.max_height);
    auto & filterResult = refineResults[i];
    dmf.refineDepthMap(filterProblem, filterResult);
  }

  /* mean filter and resize */
  BOOST_LOG_TRIVIAL(info) << "start resize";
  for(auto i = 0; i < project_info.image_pairs.size(); i++){
    BOOST_LOG_TRIVIAL(info) << "resize filter " << i << " ... ";
    FilterProblem filterProblem;
    filterProblem.ref_pose = project_info.image_poses[i];
    filterProblem.ref_K = project_info.image_Ks[i];
    filterProblem.ref_rgb = refineResults[i].rgb;
    filterProblem.ref_depth = refineResults[i].d;
    filterProblem.ref_prob = refineResults[i].p;
    filterProblem.visible_view = filterResults[i].v;

    for(const auto it: project_info.image_pairs[i]){
      filterProblem.src_poses.push_back(project_info.image_poses[it]);
      filterProblem.src_Ks.push_back(project_info.image_Ks[it]);
      filterProblem.src_rgbs.emplace_back(refineResults[it].rgb);
      filterProblem.src_depths.emplace_back(refineResults[it].d);
      filterProblem.src_probs.emplace_back(refineResults[it].p);
    }
    DepthMapFilter dmf(project_info.max_width, project_info.max_height);
    FilterResult filterResult;
    dmf.meanFilter(filterProblem, filterResult);

    Eigen::Matrix3d scaledK = filterProblem.ref_K*2;
    scaledK(2, 2) = 1;
    for(auto j = 0; j < filterResult.d.rows; j++){
      for(auto k = 0; k < filterResult.d.cols; k++){
        auto d = filterResult.d.at<float>(j, k);
        auto p = filterResult.p.at<float>(j, k);
        if(p > 0.5){
          Eigen::Vector3d pos = filterProblem.ref_pose.inverse()* (scaledK.inverse()*Eigen::Vector3d((k + 0.5) * d, (j + 0.5) * d, d));
          auto &point = fused_points.emplace_back();
          point.x = pos.x();
          point.y = pos.y();
          point.z = pos.z();
          const auto &rgb = filterResult.rgb.at<cv::Vec3b>(j, k);
          point.b = rgb(0);
          point.g = rgb(1);
          point.r = rgb(2);
        }
      }
    }
  }

  BOOST_LOG_TRIVIAL(info) << "write ply file start ...";
  WriteTextPlyPoints("fused.ply", fused_points, false, true);
  BOOST_LOG_TRIVIAL(info) << "write ply file done !!!";
  return 0;
}
