#pragma once
#include "../binding_helpers.h"

namespace breeze::js {
struct test {
  static async_simple::coro::Lazy<int> testAsync();
};
} // namespace breeze::js