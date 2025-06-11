#pragma once
#include <string>
#include <vector>

namespace async_simple::coro {
template <typename T> class Lazy;
}

namespace breeze::js {
struct filesystem {
  static std::string readFileAsStringSync(std::string path);
  static async_simple::coro::Lazy<std::string>
  readFileAsString(std::string path);

  struct ReadDirOptions {
    bool recursive;
    bool follow_symlinks;
  };

  static async_simple::coro::Lazy<std::vector<std::string>>
  readdir(std::string path, ReadDirOptions options = {.recursive = false,
                                                      .follow_symlinks = true});

  static std::vector<std::string> readdirSync(
      std::string path,
      ReadDirOptions options = {.recursive = false, .follow_symlinks = true});

  struct MkDirOptions {
    bool recursive;
  };

  static async_simple::coro::Lazy<bool>
  mkdir(std::string path, MkDirOptions options = {.recursive = false});

  static bool mkdirSync(std::string path,
                        MkDirOptions options = {.recursive = false});

  static bool exists(std::string path);

  struct RmOptions {
    bool recursive;
  };

  static async_simple::coro::Lazy<bool>
  rm(std::string path, RmOptions options = {.recursive = false});

  static bool rmSync(std::string path,
                     RmOptions options = {.recursive = false});

  static async_simple::coro::Lazy<bool> writeStringToFile(std::string path, std::string content);
};
} // namespace breeze::js