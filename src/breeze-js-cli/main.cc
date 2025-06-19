#include "cxxopts.hpp"
#include "quickjs/quickjs.h"
#include "breeze-js/script.h"
#include <optional>
#include <string>

int main(int argc, char **argv) {
  cxxopts::Options options("breeze-js", "Breeze.JS - A JavaScript runtime.");

  options.add_options()("f,file", "Execute a single JavaScript file",
                        cxxopts::value<std::string>())(
      "d,folder", "Monitor a folder for JavaScript files",
      cxxopts::value<std::string>())("e,eval",
                                     "Execute a string of JavaScript code",
                                     cxxopts::value<std::string>())(
      "v,version", "Print version information")("h,help", "Print usage")(
      "input", "Input file or folder",
      cxxopts::value<std::string>()); // Positional argument

  options.parse_positional({"input"});

  try {
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      return EXIT_SUCCESS;
    }
    if (result.count("version")) {
      std::cout << "Breeze.JS Version 0.1.0"
                << std::endl; // TODO: Get version dynamically
      return EXIT_SUCCESS;
    }

    auto ctx = std::make_shared<breeze::script_context>();
    ctx->reset_runtime();

    std::optional<std::string> input_file;
    if (result.count("file")) {
      input_file = result["file"].as<std::string>();
    } else if (result.count("input")) {
      input_file = result["input"].as<std::string>();
    }

    if (input_file) {
      std::string file_path = result["file"].as<std::string>();
      if (std::filesystem::exists(file_path)) {
        auto eval_result = ctx->eval_file(file_path);
        if (!eval_result.has_value()) {
          std::cerr << "Error executing file: " << eval_result.error()
                    << std::endl;
          return EXIT_FAILURE;
        }

        while (JS_PromiseState(ctx->js->ctx, eval_result->v) ==
               JS_PROMISE_PENDING) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
      } else {
        std::cerr << "Error: File not found: " << file_path << std::endl;
        return EXIT_FAILURE;
      }
    } else if (result.count("folder")) {
      std::string folder_path = result["folder"].as<std::string>();
      if (std::filesystem::exists(folder_path) &&
          std::filesystem::is_directory(folder_path)) {
        ctx->watch_folder(folder_path);
        while (true) {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
      } else {
        std::cerr << "Error: Folder not found or is not a directory: "
                  << folder_path << std::endl;
        return EXIT_FAILURE;
      }
    } else if (result.count("eval")) {
      std::string eval_string = result["eval"].as<std::string>();
      auto eval_result = ctx->eval_string(eval_string);
      if (!eval_result.has_value()) {
        std::cerr << "Error evaluating string: " << eval_result.error()
                  << std::endl;
        return EXIT_FAILURE;
      }
    } else {
      std::cout << options.help() << std::endl;
    }

  } catch (const cxxopts::exceptions::exception &e) {
    std::cerr << "Error parsing options: " << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}