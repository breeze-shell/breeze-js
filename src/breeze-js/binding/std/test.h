#pragma once
namespace async_simple::coro {
template <typename T> class Lazy;
}

namespace breeze::js {
struct test {
  static async_simple::coro::Lazy<int> testAsync();
};
} // namespace breeze::js