#pragma once
#include <string>

namespace breeze::js {
struct filesystem {
  static std::string readFileSync(std::string path);
  
};
} // namespace breeze::js