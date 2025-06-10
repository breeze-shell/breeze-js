#pragma once
#include <functional>
#include <string>

namespace async_simple::coro {
template <typename T> class Lazy;
}

namespace breeze::js {
struct infra {
  static async_simple::coro::Lazy<void> sleep(int ms);

  // This is a blocking sleep, not recommended in JS context
  // Use sleep() instead for non-blocking sleep
  static void sleepSync(int ms);

  // Breeze uses a low resolution timer (30ms) for setTimeout and setInterval
  // due to performance concerns.
  // Use sleep() for more precise timing if needed.
  static int setTimeout(std::function<void()> callback, int ms);
  // Breeze uses a low resolution timer (30ms) for setTimeout and setInterval
  // due to performance concerns.
  // Use sleep() for more precise timing if needed.
  static int setInterval(std::function<void()> callback, int ms);
  static void clearTimeout(int id);
  static void clearInterval(int id);

  static std::string atob(std::string base64);
  static std::string btoa(std::string str);
};
} // namespace breeze::js