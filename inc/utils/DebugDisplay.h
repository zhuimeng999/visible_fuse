//
// Created by lucius on 8/27/20.
//

#ifndef VISIBLE_FUSE_DEBUGDISPLAY_H
#define VISIBLE_FUSE_DEBUGDISPLAY_H

#include "log/log.h"
#include <opencv2/highgui.hpp>

class DebugDisplay {
public:
  std::string _winname;
  cv::Mat _img;
  DebugDisplay(const std::string& winname, const cv::Mat &img): _winname(winname), _img(img){
    cv::namedWindow(_winname);
    cv::setMouseCallback(_winname, mouseHandler, this);
    while(true){
      cv::imshow("debug", img);
      int key = cv::waitKey(100);
      if(key  == 'q'){
        break;
      }
    }
  }

  ~DebugDisplay(){
    cv::destroyWindow(_winname);
  }

  static void mouseHandler(int event, int x, int y, int flags, void* data){
    DebugDisplay *self = static_cast<DebugDisplay*>(data);
    if(event == cv::EVENT_MOUSEMOVE){
      if(self->_img.type() == CV_8UC3){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_img.at<cv::Vec3b>(y, x);
      } else if(self->_img.type() == CV_8UC4){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_img.at<cv::Vec4b>(y, x);
      } else if(self->_img.type() == CV_32FC1){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_img.at<float>(y, x);
      } else if(self->_img.type() == CV_32FC4){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_img.at<cv::Vec4f>(y, x);
      }
    }
  }
};

class DebugDisplay2 {
public:
  std::string _winname;
  cv::Mat _ref;
  cv::Mat _src;
  cv::Mat _diff;
  DebugDisplay2(const std::string& winname, const cv::Mat &ref, const cv::Mat &src): _winname(winname), _ref(ref), _src(src), _diff(ref - src){
    cv::namedWindow(_winname + "_ref");
    cv::namedWindow(_winname + "_src");
    cv::setMouseCallback(_winname + "_ref", mouseHandler, this);
    cv::setMouseCallback(_winname + "_src", mouseHandler, this);
    while(true){
      cv::imshow(_winname + "_ref", _ref);
      cv::imshow(_winname + "_src", _src);
      int key = cv::waitKey(100);
      if(key  == 'q'){
        break;
      }
    }
  }

  ~DebugDisplay2(){
    cv::destroyWindow(_winname + "_ref");
  }

  static void mouseHandler(int event, int x, int y, int flags, void* data){
    DebugDisplay2 *self = static_cast<DebugDisplay2*>(data);
    if(event == cv::EVENT_MOUSEMOVE){
      if(self->_ref.type() == CV_8UC3){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: ref " << self->_ref.at<cv::Vec3b>(y, x)
                << " diff " << self->_diff.at<cv::Vec3b>(y, x);
      } else if(self->_ref.type() == CV_8UC4){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_ref.at<cv::Vec4b>(y, x)
                  << " diff " << self->_diff.at<cv::Vec4b>(y, x);
      } else if(self->_ref.type() == CV_32FC1){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_ref.at<float>(y, x)
                  << " diff " << self->_diff.at<float>(y, x);
      } else if(self->_ref.type() == CV_32FC4){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_ref.at<cv::Vec4f>(y, x)
                << " diff " << self->_diff.at<cv::Vec4f>(y, x);
      }
    }
  }
};

class DebugDisplay3 {
public:
  std::string _winname;
  cv::Mat _ref;
  cv::Mat _src;
  cv::Mat _score;
  cv::Mat _diff;
  DebugDisplay3(const std::string& winname, const cv::Mat &ref, const cv::Mat &src, const cv::Mat &score): _winname(winname), _ref(ref), _src(src), _diff(ref - src), _score(score){
    cv::namedWindow(_winname + "_ref");
    cv::namedWindow(_winname + "_src");
    cv::setMouseCallback(_winname + "_ref", mouseHandler, this);
    cv::setMouseCallback(_winname + "_src", mouseHandler, this);
    while(true){
      cv::imshow(_winname + "_ref", _ref);
      cv::imshow(_winname + "_src", _src);
      int key = cv::waitKey(100);
      if(key  == 'q'){
        break;
      }
    }
  }

  ~DebugDisplay3(){
    cv::destroyWindow(_winname + "_ref");
  }

  static void mouseHandler(int event, int x, int y, int flags, void* data){
    DebugDisplay3 *self = static_cast<DebugDisplay3*>(data);
    if(event == cv::EVENT_MOUSEMOVE){
      if(self->_ref.type() == CV_8UC3){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: ref " << self->_ref.at<cv::Vec3b>(y, x)
                                << " diff " << self->_diff.at<cv::Vec3b>(y, x)
                  << " score " << self->_score.at<int>(y, x);
      } else if(self->_ref.type() == CV_8UC4){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_ref.at<cv::Vec4b>(y, x)
                                << " diff " << self->_diff.at<cv::Vec4b>(y, x)
                                << " score " << self->_score.at<int>(y, x);
      } else if(self->_ref.type() == CV_32FC1){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_ref.at<float>(y, x)
                                << " diff " << self->_diff.at<float>(y, x)
                  << " score " << self->_score.at<int>(y, x);
      } else if(self->_ref.type() == CV_32FC4){
        BOOST_LOG_TRIVIAL(info) << "Position: (" <<x << ", " << y << "), value: " << self->_ref.at<cv::Vec4f>(y, x)
                                << " diff " << self->_diff.at<cv::Vec4f>(y, x)
                  << " score " << self->_score.at<int>(y, x);
      }
    }
  }
};

#endif //VISIBLE_FUSE_DEBUGDISPLAY_H
