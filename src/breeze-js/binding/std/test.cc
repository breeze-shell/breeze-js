#include "test.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include <print>

async_simple::coro::Lazy<int> breeze::js::test::testAsync() {
  std::println("testAsync called, sleeping for 1 second...");
  co_await async_simple::coro::sleep(std::chrono::seconds(1));
  std::println("testAsync finished sleeping, returning 42");
  co_return 42;
}
