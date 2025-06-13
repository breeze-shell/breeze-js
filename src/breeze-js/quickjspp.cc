#include "quickjspp.hpp"
#include <print>
#if defined(_WIN32) || defined(_WIN64)
#include "Windows.h"
#elif defined(__linux__) || defined(__APPLE__)
#include <thread>
#include <unistd.h>
#endif

namespace qjs {
thread_local Context *Context::current;
void wait_with_msgloop(std::function<void()> f) {
  #if defined(_WIN32) || defined(_WIN64)
  auto this_thread = GetCurrentThreadId();
  bool completed_flag = false;
  auto thread_wait = std::thread([=, &completed_flag]() {
    f();
    completed_flag = true;
    PostThreadMessageW(this_thread, WM_NULL, 0, 0);
  });

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);

    if (completed_flag) {
      break;
    }
  }
  thread_wait.join();
  #else
  // We don't have a message loop in Linux or macOS, so we just run the function directly.
  f();
  #endif
}
} // namespace qjs