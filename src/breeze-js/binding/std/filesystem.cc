#include "./filesystem.h"
#include "quickjspp.hpp"
#include <filesystem>
#include <iostream>
#include <print>

namespace breeze::js {

std::string filesystem::readFileSync(std::string path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Error opening file: " + path);
  }
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  return content;
}
} // namespace breeze::js