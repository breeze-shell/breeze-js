#pragma once
#include "../binding_helpers.h"


namespace breeze::js {
struct filesystem {
  static std::string readFileAsStringSync(std::string path);
  static async_simple::coro::Lazy<std::string>
  readFileAsString(std::string path);

  struct ReadDirOptions {
    bool recursive = false;
    bool follow_symlinks = false;
  };

  static async_simple::coro::Lazy<std::vector<std::string>>
  readdir(std::string path, std::optional<ReadDirOptions> options);

  static std::vector<std::string>
  readdirSync(std::string path, std::optional<ReadDirOptions> options);

  struct MkDirOptions {
    bool recursive = false;
  };

  static async_simple::coro::Lazy<bool>
  mkdir(std::string path, std::optional<MkDirOptions> options);

  static bool mkdirSync(std::string path, std::optional<MkDirOptions> options);

  static bool exists(std::string path);

  struct RmOptions {
    bool recursive = false;
  };

  static async_simple::coro::Lazy<bool> rm(std::string path,
                                           std::optional<RmOptions> options);

  static bool rmSync(std::string path, std::optional<RmOptions> options);

  static async_simple::coro::Lazy<bool> writeStringToFile(std::string path,
                                                          std::string content);

  // Binary file I/O (returns/accepts ArrayBuffer on JS side)
  static std::vector<uint8_t> readFileSync(std::string path);
  static async_simple::coro::Lazy<std::vector<uint8_t>>
  readFile(std::string path);
  static async_simple::coro::Lazy<bool> writeFile(std::string path,
                                                   std::vector<uint8_t> content);
};
} // namespace breeze::js