#include "breeze-js/script.h"
#include "async_simple/coro/Lazy.h"
#include "binding/binding_types.breezejs.qjs.h"

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
#include "breeze-js/quickjs.h"
#include "breeze-js/quickjspp.hpp"

namespace breeze {

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
  #ifdef _WIN32
  WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), ws.size(), nullptr,
                nullptr);
  #else
  std::cout << msg << std::endl;
  #endif
}

void println(qjs::rest<std::string> args) {
  std::stringstream ss;
  for (auto &arg : args) {
    ss << arg << " ";
  }
  ss << std::endl;
  auto ws = utf8_to_wstring(ss.str());
  #ifdef _WIN32
  WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), ws.c_str(), ws.size(), nullptr,
                nullptr);
  #else
  std::cout << ss.str() << std::endl;
  #endif
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

  std::ignore = eval_string(R"(
import * as breeze from "breeze";
globalThis.breeze = breeze;
globalThis.console = {
  log: breeze.println,
  info: breeze.println,
  warn: breeze.println,
  error: breeze.println,
  debug: breeze.println,
};

globalThis.setTimeout = breeze.infra.setTimeout;
globalThis.clearTimeout = breeze.infra.clearTimeout;
globalThis.setInterval = breeze.infra.setInterval;
globalThis.clearInterval = breeze.infra.clearInterval;
globalThis.atob = breeze.infra.atob;
globalThis.btoa = breeze.infra.btoa;
globalThis.URL = breeze.infra.URL;
globalThis.URLSearchParams = breeze.infra.URLSearchParams;


    )");

  for (auto &fn : on_bind) {
    fn();
  }
}
script_context::script_context() : rt{}, js{} {}

void script_context::post(std::function<void()> task) {
  task_queue.enqueue(std::move(task));
  task_queue_size.fetch_add(1, std::memory_order_release);
  task_queue_cv.notify_one();
}

bool script_context::is_js_thread() const {
  return std::this_thread::get_id() == js_thread_id_;
}

void script_context::reset_runtime() {
  *stop_signal = true;
  task_queue_cv.notify_all();
  stop_signal = std::make_shared<int>(false);

  if (js_thread)
    js_thread->join();

  std::promise<void> p_finished;

  auto future = p_finished.get_future();

  js_thread = std::thread([&, this, ss = stop_signal]() {
    js_thread_id_ = std::this_thread::get_id();

    rt = std::make_shared<qjs::Runtime>();
    JS_UpdateStackTop(rt->rt);
    JS_SetRuntimeOpaque(rt->rt, this);

    js = std::make_shared<qjs::Context>(*rt);
    js->script_ctx = this;

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

    while (!*ss) {
      // Drain all pending tasks (JS jobs + C++ tasks via JS_ExecutePendingJob)
      JSContext *ctx1;
      while (JS_ExecutePendingJob(rt->rt, &ctx1) > 0 && !*ss) {
        // keep draining
      }

      if (*ss)
        break;

      // Wait for new tasks or stop signal
      std::unique_lock lock(cv_mutex);
      task_queue_cv.wait(lock, [&]() {
        return task_queue_size.load(std::memory_order_acquire) > 0 || *ss;
      });
    }
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
script_context::eval_string_impl(const std::string &script,
                            std::string_view filename) {
  try {
    JS_UpdateStackTop(rt->rt);
    auto func = JS_Eval(js->ctx, script.c_str(), script.size(), filename.data(),
                        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

    if (JS_IsException(func)) {
      auto error_val = js->getException();
      std::string error_msg = "Error compiling file: " + std::string(filename) +
                              " " + error_val.as<std::string>() +
                              error_val["stack"].as<std::string>();
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
                              " " + val.as<std::string>() +
                              val["stack"].as<std::string>();
      return std::unexpected(error_msg);
    }
    return val;
  } catch (std::exception &e) {
    std::string error_msg =
        "Exception in file: " + std::string(filename) + " " + e.what();
    return std::unexpected(error_msg);
  }
}

std::expected<qjs::Value, std::string>
script_context::eval_string(const std::string &script,
                            std::string_view filename) {
  if (is_js_thread()) {
    return eval_string_impl(script, filename);
  }

  return post_sync([this, script, filename = std::string(filename)]() {
    return eval_string_impl(script, filename);
  });
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
    return std::format("JS Error: {}\nStack: {}", exc.as<std::string>(),
                       exc["stack"].as<std::string>());
  }

  return "No exception occurred";
}
script_context::~script_context() {
  if (js_thread && js_thread->joinable()) {
    *stop_signal = true;
    task_queue_cv.notify_all();
    js_thread->join();
  }
  // Drain any remaining tasks that were posted after the JS thread stopped
  // (e.g., Value destructors posting JS_FreeValue from other threads).
  // We run them here on the current thread since the JS thread is gone
  // and we're about to destroy the runtime anyway.
  std::function<void()> task;
  while (task_queue.try_dequeue(task)) {
    task();
  }
}
} // namespace breeze
