#pragma once
#include "quickjspp.hpp"
#include <chrono>
#include <condition_variable>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <thread>
#include <unordered_map>
#include <unordered_set>


namespace breeze {
extern thread_local bool is_thread_js_main;
struct script_context {
  std::shared_ptr<qjs::Runtime> rt;
  std::shared_ptr<qjs::Context> js;
  std::shared_ptr<int> stop_signal = std::make_shared<int>(0);
  std::optional<std::jthread> js_thread;
  std::filesystem::path module_base;

  std::unordered_set<std::function<void()>> on_bind;

  script_context();
  ~script_context();
  void bind();

  void reset_runtime();
  std::expected<qjs::Value, std::string>
  eval_file(const std::filesystem::path &path);
  std::expected<qjs::Value, std::string>
  eval_string(const std::string &script, std::string_view filename = "<eval>");
  std::string current_exception_string();
  void watch_folder(
      const std::filesystem::path &path,
      std::function<bool()> on_reload = []() { return true; });
};
} // namespace breeze