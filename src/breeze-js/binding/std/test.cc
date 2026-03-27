#include "test.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include <cstdio>

async_simple::coro::Lazy<int> breeze::js::test::testAsync() {
  std::printf("testAsync called, sleeping for 1 second...\n");
  co_await async_simple::coro::sleep(std::chrono::seconds(1));
  std::printf("testAsync finished sleeping, returning 42\n");
  co_return 42;
}
