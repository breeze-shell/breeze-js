#pragma once
#include "./platform_thread.h"
#include "./quickjspp.hpp"
#include <atomic>
#include <chrono>
#include <concurrentqueue.h>
#include <condition_variable>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace breeze {
struct script_context {
  std::shared_ptr<qjs::Runtime> rt;
  std::shared_ptr<qjs::Context> js;
  std::optional<platform_thread> js_thread;
  std::filesystem::path module_base;

  // Lock-free task queue: all tasks (JS jobs + C++ tasks) go through here.
  moodycamel::ConcurrentQueue<std::function<void()>> task_queue;
  std::atomic<size_t> task_queue_size{0};
  std::condition_variable task_queue_cv;
  std::mutex cv_mutex;

  std::vector<std::function<void()>> on_bind;

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

  void post(std::function<void()> task);
  bool is_js_thread() const;
  void run_event_loop();
  void stop_event_loop_in_time(std::chrono::milliseconds timeout);

  // Set before signalling stop to give the JS thread a grace period to drain.
  std::optional<std::chrono::steady_clock::time_point> shutdown_deadline;

  template <typename F>
  auto post_sync(F &&f) -> decltype(f()) {
    using R = decltype(f());
    if (is_js_thread()) {
      return f();
    }
    if constexpr (std::is_void_v<R>) {
      std::promise<void> promise;
      auto future = promise.get_future();
      post([&promise, &f]() {
        try {
          f();
          promise.set_value();
        } catch (...) {
          promise.set_exception(std::current_exception());
        }
      });
      qjs::wait_with_msgloop([&future]() { future.wait(); });
      return future.get();
    } else {
      std::promise<R> promise;
      auto future = promise.get_future();
      post([&promise, &f]() {
        try {
          promise.set_value(f());
        } catch (...) {
          promise.set_exception(std::current_exception());
        }
      });
      qjs::wait_with_msgloop([&future]() { future.wait(); });
      return future.get();
    }
  }

private:
  std::expected<qjs::Value, std::string>
  eval_string_impl(const std::string &script, std::string_view filename);
  platform_thread::id js_thread_id_;
};
} // namespace breeze
