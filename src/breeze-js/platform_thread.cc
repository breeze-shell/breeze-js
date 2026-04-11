#include "breeze-js/platform_thread.h"

#include <exception>
#include <memory>
#include <utility>

#if defined(_WIN32)
#include <process.h>
#include <cerrno>
#else
#include <cerrno>
#include <cstring>
#endif

namespace breeze {
namespace {

struct thread_start_data {
  explicit thread_start_data(std::function<void()> fn) : fn(std::move(fn)) {}
  std::function<void()> fn;
};

#if defined(_WIN32)
unsigned __stdcall thread_entry(void *arg) {
  std::unique_ptr<thread_start_data> data(
      static_cast<thread_start_data *>(arg));
  try {
    data->fn();
  } catch (...) {
    std::terminate();
  }
  return 0;
}
#else
void *thread_entry(void *arg) {
  std::unique_ptr<thread_start_data> data(
      static_cast<thread_start_data *>(arg));
  try {
    data->fn();
  } catch (...) {
    std::terminate();
  }
  return nullptr;
}
#endif

[[noreturn]] void throw_system_error(int code, const char *message) {
  throw std::system_error(code, std::system_category(), message);
}

} // namespace

bool operator==(const platform_thread::id &lhs,
                const platform_thread::id &rhs) noexcept {
  if (lhs.valid_ != rhs.valid_) {
    return false;
  }
  if (!lhs.valid_) {
    return true;
  }
#if defined(_WIN32)
  return lhs.value_ == rhs.value_;
#else
  return pthread_equal(lhs.value_, rhs.value_) != 0;
#endif
}

platform_thread::platform_thread(std::function<void()> fn,
                                 std::size_t stack_size_bytes) {
  auto data = std::make_unique<thread_start_data>(std::move(fn));

#if defined(_WIN32)
  unsigned thread_id = 0;
  auto handle = reinterpret_cast<HANDLE>(
      _beginthreadex(nullptr, static_cast<unsigned>(stack_size_bytes),
                     thread_entry, data.get(), 0, &thread_id));
  if (!handle) {
    throw_system_error(errno, "_beginthreadex failed");
  }
  handle_ = handle;
  thread_id_ = static_cast<DWORD>(thread_id);
  data.release();
#else
  pthread_attr_t attr;
  auto err = pthread_attr_init(&attr);
  if (err != 0) {
    throw_system_error(err, "pthread_attr_init failed");
  }

  if (stack_size_bytes != 0) {
    err = pthread_attr_setstacksize(&attr, stack_size_bytes);
    if (err != 0) {
      pthread_attr_destroy(&attr);
      throw_system_error(err, "pthread_attr_setstacksize failed");
    }
  }

  err = pthread_create(&handle_, &attr, thread_entry, data.get());
  pthread_attr_destroy(&attr);
  if (err != 0) {
    throw_system_error(err, "pthread_create failed");
  }
  joinable_ = true;
  data.release();
#endif
}

platform_thread::platform_thread(platform_thread &&other) noexcept {
  *this = std::move(other);
}

platform_thread &platform_thread::operator=(platform_thread &&other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (joinable()) {
    std::terminate();
  }
#if defined(_WIN32)
  handle_ = std::exchange(other.handle_, nullptr);
  thread_id_ = std::exchange(other.thread_id_, 0);
#else
  handle_ = other.handle_;
  joinable_ = std::exchange(other.joinable_, false);
#endif
  return *this;
}

platform_thread::~platform_thread() {
  if (joinable()) {
    std::terminate();
  }
}

bool platform_thread::joinable() const noexcept {
#if defined(_WIN32)
  return handle_ != nullptr;
#else
  return joinable_;
#endif
}

void platform_thread::join() {
  if (!joinable()) {
    throw std::system_error(std::make_error_code(std::errc::invalid_argument));
  }
#if defined(_WIN32)
  auto wait_result = WaitForSingleObject(handle_, INFINITE);
  if (wait_result != WAIT_OBJECT_0) {
    throw_system_error(GetLastError(), "WaitForSingleObject failed");
  }
  if (!CloseHandle(handle_)) {
    throw_system_error(GetLastError(), "CloseHandle failed");
  }
#else
  auto err = pthread_join(handle_, nullptr);
  if (err != 0) {
    throw_system_error(err, "pthread_join failed");
  }
#endif
  reset();
}

platform_thread::id platform_thread::get_id() const noexcept {
  if (!joinable()) {
    return {};
  }
#if defined(_WIN32)
  return id(thread_id_);
#else
  return id(handle_);
#endif
}

platform_thread::id platform_thread::current_id() noexcept {
#if defined(_WIN32)
  return id(GetCurrentThreadId());
#else
  return id(pthread_self());
#endif
}

void platform_thread::reset() noexcept {
#if defined(_WIN32)
  handle_ = nullptr;
  thread_id_ = 0;
#else
  handle_ = pthread_t{};
  joinable_ = false;
#endif
}

} // namespace breeze
