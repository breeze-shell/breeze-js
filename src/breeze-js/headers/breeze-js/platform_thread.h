#pragma once

#include <cstddef>
#include <functional>
#include <system_error>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) ||      \
    defined(__OpenBSD__) || defined(__NetBSD__)
#include <pthread.h>
#else
#error "platform_thread is not implemented for this platform"
#endif

namespace breeze {

class platform_thread {
public:
  class id {
  public:
    id() noexcept = default;

    friend bool operator==(const id &lhs, const id &rhs) noexcept;

  private:
    friend class platform_thread;

#if defined(_WIN32)
    explicit id(DWORD value) noexcept : value_(value), valid_(true) {}
    DWORD value_{0};
#else
    explicit id(pthread_t value) noexcept : value_(value), valid_(true) {}
    pthread_t value_{};
#endif
    bool valid_{false};
  };

  platform_thread() noexcept = default;
  explicit platform_thread(std::function<void()> fn,
                           std::size_t stack_size_bytes = 0);
  platform_thread(platform_thread &&other) noexcept;
  platform_thread &operator=(platform_thread &&other) noexcept;

  platform_thread(const platform_thread &) = delete;
  platform_thread &operator=(const platform_thread &) = delete;

  ~platform_thread();

  bool joinable() const noexcept;
  void join();
  id get_id() const noexcept;

  static id current_id() noexcept;

private:
  void reset() noexcept;

#if defined(_WIN32)
  HANDLE handle_{nullptr};
  DWORD thread_id_{0};
#else
  pthread_t handle_{};
  bool joinable_{false};
#endif
};

} // namespace breeze
