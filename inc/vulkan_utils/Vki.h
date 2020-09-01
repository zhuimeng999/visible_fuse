//
// Created by lucius on 8/1/20.
//

#ifndef UNTITLED_VKI_H
#define UNTITLED_VKI_H

#include <vulkan/vulkan.hpp>

class Vki {
public:
  explicit Vki(bool enableValidate);
  ~Vki();

public:
  VkInstance instance;
  std::vector<VkPhysicalDevice> phyDevices;

private:
  bool isValidateEnabled;
  VkDebugReportCallbackEXT debugReportCallback;

  std::vector<VkLayerProperties> availableLayers;
  inline bool isLayerAvailable(std::vector<const char *> &requestLayer);
};


#endif //UNTITLED_VKI_H
