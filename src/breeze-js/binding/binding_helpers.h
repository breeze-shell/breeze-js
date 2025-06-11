#pragma once
#include <optional>
#include <string>
#include <vector>

namespace async_simple::coro {
template <typename T> class Lazy;
}

template <typename T>
inline void setDefault(std::optional<T> &opt, const T &defaultValue) {
  if (!opt.has_value()) {
    opt = defaultValue;
  }
}