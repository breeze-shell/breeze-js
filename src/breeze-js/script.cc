#include "script.h"
#include "async_simple/coro/Lazy.h"
#include "binding/binding_qjs.h"

#include <algorithm>
#include <codecvt>
#include <future>
#include <iostream>
#include <mutex>
#include <print>
#include <ranges>
#include <sstream>
#include <string_view>
#include <thread>
#include <unordered_set>

#include "FileWatch.hpp"
#include "quickjs.h"
#include "quickjspp.hpp"

namespace breeze {

thread_local bool is_thread_js_main = false;
std::wstring utf8_to_wstring(const std::string &str) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  try {
    return converter.from_bytes(str);
  } catch (const std::range_error &e) {
    std::cerr << "Error converting UTF-8 to wide string: " << e.what()
              << std::endl;
    return L"";
  }
}

std::string wstring_to_utf8(const std::wstring &wstr) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
  try {
    return converter.to_bytes(wstr);
  } catch (const std::range_error &e) {
    std::cerr << "Error converting wide string to UTF-8: " << e.what()
              << std::endl;
    return "";
  }
}

void dbgout(const std::string &msg) {
  auto ws = utf8_to_wstring(msg);
  WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), ws.size(), nullptr,
                nullptr);
}

void println(qjs::rest<std::string> args) {
  std::stringstream ss;
  for (auto &arg : args) {
    ss << arg << " ";
  }
  ss << std::endl;
  auto ws = utf8_to_wstring(ss.str());
  WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), ws.size(), nullptr,
                nullptr);
}

void script_context::bind() {
  auto &module = js->addModule("breeze");

  module.function("println", println);

  bindAll(module);

  auto g = js->global();
  g["console"] = js->newObject();
  qjs::Value println_fn =
      qjs::js_traits<std::function<void(qjs::rest<std::string>)>>::wrap(
          js->ctx, println);
  g["console"]["log"] = println_fn;
  g["console"]["info"] = println_fn;
  g["console"]["warn"] = println_fn;
  g["console"]["error"] = println_fn;
  g["console"]["debug"] = println_fn;
}
script_context::script_context() : rt{}, js{} {}
void script_context::reset_runtime() {
  *stop_signal = true;
  stop_signal = std::make_shared<int>(false);

  if (js_thread)
    js_thread->join();

  std::promise<void> p_finished;

  auto future = p_finished.get_future();

  js_thread = std::jthread([&, this, ss = stop_signal]() {
    is_thread_js_main = true;
    rt = std::make_shared<qjs::Runtime>();
    JS_UpdateStackTop(rt->rt);
    js = std::make_shared<qjs::Context>(*rt);

    js->moduleLoader = [&](std::string_view module_name) {
      auto module_path = module_base / (std::string(module_name) + ".js");
      if (!std::filesystem::exists(module_path)) {
        return qjs::Context::ModuleData{};
      }
      std::ifstream file(module_path);
      std::string script((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
      return qjs::Context::ModuleData{script};
    };

    bind();

    p_finished.set_value();

    while (auto ptr = js) {
      if (ptr->ctx) {
        while (JS_IsJobPending(rt->rt) && !*ss) {
          auto ctx = ptr->ctx;
          auto ctx1 = ctx;
          auto res = JS_ExecutePendingJob(rt->rt, &ctx1);
          if (res == -999) {
            dbgout("JS loop critical error! Restarting...");
            return;
          }
          std::lock_guard lock(ptr->js_job_start_mutex);
          ptr->has_pending_job = JS_IsJobPending(rt->rt);
        }
      }
      if (*ss)
        break;
      std::unique_lock lock(js->js_job_start_mutex);
      if (js->has_pending_job)
        continue;
      js->js_job_start_cv.wait_for(lock, std::chrono::milliseconds(100), [&]() {
        return js->has_pending_job || *ss;
      });
    }
    is_thread_js_main = false;
  });

  future.wait();
}

std::expected<qjs::Value, std::string>
script_context::eval_file(const std::filesystem::path &path) {
  try {
    std::ifstream file(path);
    if (!file.is_open()) {
      return std::unexpected("Error opening file: " + path.generic_string());
    }
    std::string script((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    const auto path_str = path.generic_string();
    return eval_string(script, path_str);
  } catch (std::exception &e) {
    return std::unexpected("Exception reading file: " + path.generic_string() +
                           ": " + e.what());
  }
}

std::expected<qjs::Value, std::string>
script_context::eval_string(const std::string &script,
                            std::string_view filename) {
  try {
    JS_UpdateStackTop(rt->rt);
    auto func = JS_Eval(js->ctx, script.c_str(), script.size(), filename.data(),
                        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(func)) {
      auto error_val = js->getException();
      std::string error_msg = "Error compiling file: " + std::string(filename) +
                              " " + (std::string)error_val +
                              (std::string)error_val["stack"];
      JS_FreeValue(js->ctx, func);
      return std::unexpected(error_msg);
    }

    JSModuleDef *m = (JSModuleDef *)JS_VALUE_GET_PTR(func);
    auto meta_obj = JS_GetImportMeta(js->ctx, m);

    JS_DefinePropertyValueStr(js->ctx, meta_obj, "url",
                              JS_NewString(js->ctx, filename.data()),
                              JS_PROP_C_W_E);

    const auto filename_str =
        std::filesystem::path(filename).filename().string();
    JS_DefinePropertyValueStr(js->ctx, meta_obj, "name",
                              JS_NewString(js->ctx, filename_str.c_str()),
                              JS_PROP_C_W_E);

    JS_FreeValue(js->ctx, meta_obj);

    auto val = qjs::Value{js->ctx, JS_EvalFunction(js->ctx, func)};
    if (val.isError()) {
      std::string error_msg = "Error executing file: " + std::string(filename) +
                              " " + (std::string)val +
                              (std::string)val["stack"];
      return std::unexpected(error_msg);
    }
    return val;
  } catch (std::exception &e) {
    std::string error_msg =
        "Exception in file: " + std::string(filename) + " " + e.what();
    return std::unexpected(error_msg);
  }
}

void script_context::watch_folder(const std::filesystem::path &path,
                                  std::function<bool()> on_reload) {
  bool has_update = false;

  auto reload_all = [&]() {
    reset_runtime();

    std::vector<std::filesystem::path> files;
    std::ranges::copy(std::filesystem::directory_iterator(path) |
                          std::ranges::views::filter([](auto &entry) {
                            return entry.path().extension() == ".js";
                          }),
                      std::back_inserter(files));

    for (auto &path : files) {
      if (auto res = eval_file(path); !res) {
        std::cerr << "Error evaluating file: " << path.generic_string() << ": "
                  << res.error() << std::endl;
      } else {
      }
    }
  };

  reload_all();

  filewatch::FileWatch<std::string> watch(
      path.generic_string(),
      [&](const std::string &path, const filewatch::Event change_type) {
        if (!path.ends_with(".js")) {
          return;
        }

        has_update = true;
      });

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    if (has_update && on_reload()) {
      has_update = false;
      reload_all();
    }
  }
}

std::string script_context::current_exception_string() {
  if (!js || !js->ctx) {
    return "No JS context available";
  }

  auto exc = js->getException();
  if (exc.isError()) {
    return std::format("JS Error: {}\nStack: {}", (std::string)exc,
                       (std::string)exc["stack"]);
  }

  return "No exception occurred";
}
script_context::~script_context() {
  if (js_thread && js_thread->joinable()) {
    *stop_signal = true;
    js_thread->join();
  }
}
} // namespace breeze
