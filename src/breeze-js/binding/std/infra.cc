#include "infra.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include <chrono>
#include <print>
#include <thread>


#include "breeze-js/quickjspp.hpp"
namespace breeze::js {
async_simple::coro::Lazy<void> infra::sleep(int ms) {
  co_await async_simple::coro::sleep(std::chrono::milliseconds(ms));
  co_return;
}
void infra::sleepSync(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct Timer {
  std::function<void()> callback;
  std::weak_ptr<qjs::Context> ctx;
  int delay;
  int elapsed = 0;
  bool repeat;
  int id;
};

std::list<std::unique_ptr<Timer>> timers;
std::optional<std::thread> timer_thread;

void timer_thread_func() {
  while (true) {
    constexpr auto sleep_time = 30;
    Sleep(sleep_time);

    std::vector<std::function<void()>> callbacks;
    for (auto &timer : timers) {
      if (!timer || timer->ctx.expired()) {
        timer = nullptr;
        continue;
      }
      timer->elapsed += sleep_time;
      if (timer->elapsed >= timer->delay) {

        bool repeat = timer->repeat;
        timer->elapsed = 0;
        callbacks.push_back(timer->callback);
        if (!repeat) {
          timer = nullptr;
        }
      }
    }

    timers.erase(std::remove_if(timers.begin(), timers.end(),
                                [](const auto &timer) { return !timer; }),
                 timers.end());

    for (const auto &callback : callbacks) {
      try {
        callback();
      } catch (std::exception &e) {
        std::cerr << "Error in timer callback: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Unknown in timer callback: " << std::endl;
      }
    }
  }
}

void ensure_timer_thread() {
  if (!timer_thread) {
    timer_thread = std::thread(timer_thread_func);
    timer_thread->detach();
  }
}

int infra::setTimeout(std::function<void()> callback, int delay) {
  ensure_timer_thread();

  auto timer = std::make_unique<Timer>();
  timer->callback = callback;
  timer->delay = delay;
  timer->repeat = false;
  timer->ctx = qjs::Context::current->weak_from_this();
  timer->id = timers.size() + 1;
  auto id = timer->id;
  timers.push_back(std::move(timer));

  return id;
};
void infra::clearTimeout(int id) {
  if (auto it = std::find_if(
          timers.begin(), timers.end(),
          [id](const auto &timer) { return timer && timer->id == id; });
      it != timers.end()) {
    timers.erase(it);
  }
};
int infra::setInterval(std::function<void()> callback, int delay) {
  ensure_timer_thread();

  auto timer = std::make_unique<Timer>();
  timer->callback = callback;
  timer->delay = delay;
  timer->repeat = true;
  timer->ctx = qjs::Context::current->weak_from_this();
  timer->id = timers.size() + 1;
  auto id = timer->id;
  timers.push_back(std::move(timer));

  return id;
};
void infra::clearInterval(int id) { clearTimeout(id); };

std::string infra::atob(std::string base64) {
  std::string result;
  result.reserve(base64.length() * 3 / 4);

  const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

  int val = 0, valb = -8;
  for (unsigned char c : base64) {
    if (c == '=')
      break;

    if (isspace(c))
      continue;

    val = (val << 6) + base64_chars.find(c);
    valb += 6;
    if (valb >= 0) {
      result.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return result;
}

std::string infra::btoa(std::string str) {
  const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

  std::string result;
  int val = 0, valb = -6;
  for (unsigned char c : str) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      result.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (result.size() % 4) {
    result.push_back('=');
  }
  return result;
}
}; // namespace breeze::js