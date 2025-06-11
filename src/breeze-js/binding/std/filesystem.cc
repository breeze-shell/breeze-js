#include "./filesystem.h"
#include "async_simple/coro/SyncAwait.h"
#include "cinatra/ylt/coro_io/coro_file.hpp"
#include "quickjspp.hpp"
#include <filesystem>
#include <iostream>
#include <print>

namespace breeze::js {

std::string filesystem::readFileAsStringSync(std::string path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Error opening file: " + path);
  }
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  return content;
}
async_simple::coro::Lazy<std::string>
filesystem::readFileAsString(std::string path) {
  coro_io::coro_file file(path, std::ios::in | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Error opening file: " + path);
  }

  auto size = file.file_size();

  if (size == 0) {
    co_return "";
  }

  std::string content(size, '\0');
  auto [ec, read_size] = co_await file.async_read(content.data(), size);
  if (ec) {
    throw std::runtime_error("Error reading file: " + path + " - " +
                             ec.message());
  }

  if (read_size != size) {
    throw std::runtime_error("Error reading file: " + path +
                             " - Expected size: " + std::to_string(size) +
                             ", Read size: " + std::to_string(read_size));
  }

  co_return content;
}
std::vector<std::string>
filesystem::readdirSync(std::string path,
                        std::optional<ReadDirOptions> options) {
  setDefault(options, {.recursive = false, .follow_symlinks = true});

  std::vector<std::string> result;
  try {
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
      if (options->follow_symlinks || !std::filesystem::is_symlink(entry)) {
        result.push_back(entry.path().filename().string());
      }
      if (options->recursive && entry.is_directory()) {
        auto sub_entries = readdirSync(entry.path().string(), options);
        result.insert(result.end(), sub_entries.begin(), sub_entries.end());
      }
    }
  } catch (const std::filesystem::filesystem_error &e) {
    throw std::runtime_error(
        std::format("Error reading directory '{}': {}", path, e.what()));
  }
  return result;
}

async_simple::coro::Lazy<std::vector<std::string>>
filesystem::readdir(std::string path, std::optional<ReadDirOptions> options) {
  setDefault(options, {.recursive = false, .follow_symlinks = true});
  co_return readdirSync(path, options);
}
async_simple::coro::Lazy<bool>
filesystem::mkdir(std::string path, std::optional<MkDirOptions> options) {
  setDefault(options, {.recursive = false});
  co_return mkdirSync(path, options);
}
bool filesystem::mkdirSync(std::string path,
                           std::optional<MkDirOptions> options) {
  setDefault(options, {.recursive = false});
  try {
    if (options->recursive) {
      std::filesystem::create_directories(path);
    } else {
      std::filesystem::create_directory(path);
    }
    return true;
  } catch (const std::filesystem::filesystem_error &e) {
    throw std::runtime_error(
        std::format("Error creating directory '{}': {}", path, e.what()));
  }
}
bool filesystem::exists(std::string path) {
  return std::filesystem::exists(path);
}
bool filesystem::rmSync(std::string path, std::optional<RmOptions> options) {
  setDefault(options, {.recursive = false});
  if (options->recursive) {
    std::filesystem::remove_all(path);
  } else {
    std::filesystem::remove(path);
  }

  return !std::filesystem::exists(path);
};
async_simple::coro::Lazy<bool>
filesystem::writeStringToFile(std::string path, std::string content) {
  // create first if it doesn't exist
  if (!std::filesystem::exists(path)) {
    std::filesystem::create_directories(
        std::filesystem::path(path).parent_path());
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
      throw std::runtime_error("Error creating file: " + path);
    }
    file.close();
  }
  coro_io::coro_file file(path, std::ios::out | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Error opening file for writing: " + path);
  }

  auto [ec, write_size] = co_await file.async_write(
      std::string_view(content.data(), content.size()));

  if (ec) {
    throw std::runtime_error("Error writing to file: " + path + " - " +
                             ec.message());
  }

  if (write_size != content.size()) {
    throw std::runtime_error(
        "Error writing to file: " + path +
        " - Expected size: " + std::to_string(content.size()) +
        ", Written size: " + std::to_string(write_size));
  }

  co_return true;
}
async_simple::coro::Lazy<bool>
filesystem::rm(std::string path, std::optional<RmOptions> options) {
  setDefault(options, {.recursive = false});
  co_return rmSync(path, options);
}
} // namespace breeze::js