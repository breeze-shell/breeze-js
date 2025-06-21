#include "infra.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Sleep.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <print>
#include <regex>
#include <sstream>
#include <thread>

#include "breeze-js/quickjspp.hpp"

#include "ctre.hpp"
#include "ctre/wrapper.hpp"

namespace breeze::js {
async_simple::coro::Lazy<void> infra::sleep(int ms) {
  co_await async_simple::coro::sleep(std::chrono::milliseconds(ms));
  co_return;
}
void infra::sleepSync(int ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

struct Timer {
  std::function<void()> callback;
  std::weak_ptr<qjs::Context> ctx;
  int delay;
  int elapsed = 0;
  bool repeat;
  int id;
};

std::list<std::unique_ptr<Timer>> timers;
std::optional<std::thread> timer_thread;

void timer_thread_func() {
  while (true) {
    constexpr auto sleep_time = 30;
    Sleep(sleep_time);

    std::vector<std::function<void()>> callbacks;
    for (auto &timer : timers) {
      if (!timer || timer->ctx.expired()) {
        timer = nullptr;
        continue;
      }
      timer->elapsed += sleep_time;
      if (timer->elapsed >= timer->delay) {

        bool repeat = timer->repeat;
        timer->elapsed = 0;
        callbacks.push_back(timer->callback);
        if (!repeat) {
          timer = nullptr;
        }
      }
    }

    timers.erase(std::remove_if(timers.begin(), timers.end(),
                                [](const auto &timer) { return !timer; }),
                 timers.end());

    for (const auto &callback : callbacks) {
      try {
        callback();
      } catch (std::exception &e) {
        std::cerr << "Error in timer callback: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Unknown in timer callback: " << std::endl;
      }
    }
  }
}

void ensure_timer_thread() {
  if (!timer_thread) {
    timer_thread = std::thread(timer_thread_func);
    timer_thread->detach();
  }
}

int infra::setTimeout(std::function<void()> callback, int delay) {
  ensure_timer_thread();

  auto timer = std::make_unique<Timer>();
  timer->callback = callback;
  timer->delay = delay;
  timer->repeat = false;
  timer->ctx = qjs::Context::current->weak_from_this();
  timer->id = timers.size() + 1;
  auto id = timer->id;
  timers.push_back(std::move(timer));

  return id;
};
void infra::clearTimeout(int id) {
  if (auto it = std::find_if(
          timers.begin(), timers.end(),
          [id](const auto &timer) { return timer && timer->id == id; });
      it != timers.end()) {
    timers.erase(it);
  }
};
int infra::setInterval(std::function<void()> callback, int delay) {
  ensure_timer_thread();

  auto timer = std::make_unique<Timer>();
  timer->callback = callback;
  timer->delay = delay;
  timer->repeat = true;
  timer->ctx = qjs::Context::current->weak_from_this();
  timer->id = timers.size() + 1;
  auto id = timer->id;
  timers.push_back(std::move(timer));

  return id;
};
void infra::clearInterval(int id) { clearTimeout(id); };

std::string infra::atob(std::string base64) {
  std::string result;
  result.reserve(base64.length() * 3 / 4);

  const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

  int val = 0, valb = -8;
  for (unsigned char c : base64) {
    if (c == '=')
      break;

    if (isspace(c))
      continue;

    val = (val << 6) + base64_chars.find(c);
    valb += 6;
    if (valb >= 0) {
      result.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return result;
}

std::string infra::btoa(std::string str) {
  const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

  std::string result;
  int val = 0, valb = -6;
  for (unsigned char c : str) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      result.push_back(base64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) {
    result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (result.size() % 4) {
    result.push_back('=');
  }
  return result;
}

// Helper functions for URL parsing
namespace {
std::string percent_encode(const std::string &str) {
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (auto c : str) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
      continue;
    }

    escaped << std::uppercase;
    escaped << '%' << std::setw(2) << int((unsigned char)c);
    escaped << std::nouppercase;
  }

  return escaped.str();
}

std::string percent_decode(const std::string &str) {
  std::string result;
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '%' && i + 2 < str.size()) {
      int value;
      std::istringstream iss(str.substr(i + 1, 2));
      if (iss >> std::hex >> value) {
        result += static_cast<char>(value);
        i += 2;
      } else {
        result += str[i];
      }
    } else {
      result += str[i];
    }
  }
  return result;
}

} // namespace

// URLSearchParams implementation
infra::URLSearchParams::URLSearchParams() : $url_object(nullptr) {}

infra::URLSearchParams::URLSearchParams(const std::string &init)
    : $url_object(nullptr) {
  this->$parse_query_string(init);
}

infra::URLSearchParams::URLSearchParams(
    const std::vector<std::vector<std::string>> &init)
    : $url_object(nullptr) {
  for (const auto &pair : init) {
    if (pair.size() == 2) {
      list.emplace_back(pair[0], pair[1]);
    }
  }
}

infra::URLSearchParams::URLSearchParams(
    const std::map<std::string, std::string> &init)
    : $url_object(nullptr) {
  for (const auto &pair : init) {
    list.emplace_back(pair.first, pair.second);
  }
}

size_t infra::URLSearchParams::size() const { return list.size(); }

void infra::URLSearchParams::append(const std::string &name,
                                    const std::string &value) {
  list.emplace_back(name, value);
  $update_url_query();
}

void infra::URLSearchParams::remove(const std::string &name,
                                    const std::string &value) {
  list.erase(std::remove_if(list.begin(), list.end(),
                            [&](const auto &pair) {
                              if (value.empty()) {
                                return pair.first == name;
                              }
                              return pair.first == name && pair.second == value;
                            }),
             list.end());
  $update_url_query();
}

std::string infra::URLSearchParams::get(const std::string &name) {
  auto it = std::find_if(list.begin(), list.end(),
                         [&](const auto &pair) { return pair.first == name; });
  return it != list.end() ? it->second : "";
}

std::vector<std::string>
infra::URLSearchParams::getAll(const std::string &name) {
  std::vector<std::string> result;
  for (const auto &pair : list) {
    if (pair.first == name) {
      result.push_back(pair.second);
    }
  }
  return result;
}

bool infra::URLSearchParams::has(const std::string &name,
                                 const std::string &value) {
  if (value.empty()) {
    return std::any_of(list.begin(), list.end(),
                       [&](const auto &pair) { return pair.first == name; });
  }
  return std::any_of(list.begin(), list.end(), [&](const auto &pair) {
    return pair.first == name && pair.second == value;
  });
}

void infra::URLSearchParams::set(const std::string &name,
                                 const std::string &value) {
  remove(name);
  append(name, value);
}

void infra::URLSearchParams::sort() {
  std::sort(list.begin(), list.end(),
            [](const auto &a, const auto &b) { return a.first < b.first; });
  $update_url_query();
}

std::string infra::URLSearchParams::toString() const {
  std::string result;
  for (size_t i = 0; i < list.size(); ++i) {
    if (i != 0) {
      result += "&";
    }
    result +=
        percent_encode(list[i].first) + "=" + percent_encode(list[i].second);
  }
  return result;
}

void infra::URLSearchParams::$update_url_query() {
  if ($url_object) {
    $url_object->set_search(toString());
  }
}

void infra::URLSearchParams::$parse_query_string(const std::string &query) {
  list.clear();
  std::string current = query;
  size_t pos = 0;

  while (pos < current.size()) {
    size_t amp_pos = current.find('&', pos);
    std::string pair = current.substr(pos, amp_pos - pos);

    size_t eq_pos = pair.find('=');
    std::string name =
        eq_pos != std::string::npos ? pair.substr(0, eq_pos) : pair;
    std::string value =
        eq_pos != std::string::npos ? pair.substr(eq_pos + 1) : "";

    list.emplace_back(percent_decode(name), percent_decode(value));

    pos = amp_pos != std::string::npos ? amp_pos + 1 : current.size();
  }
}

// URL implementation
infra::URL::URL(const std::string &url_string) {
  if (!$basic_url_parser(url_string)) {
    throw std::runtime_error("Failed to parse URL");
  }
  $search_params->$url_object = this;
}

std::shared_ptr<infra::URL> infra::URL::parse(const std::string &url_string) {
  try {
    return std::make_shared<URL>(url_string);
  } catch (...) {
    return nullptr;
  }
}

bool infra::URL::canParse(const std::string &url_string) {
  URL temp("");
  return temp.$basic_url_parser(url_string);
}

std::string infra::URL::get_href() const { 
  return $serialize_url(); 
}

void infra::URL::set_href(const std::string &new_href) {
  if (!$basic_url_parser(new_href)) {
    throw std::runtime_error("Invalid URL");
  }
  // Empty this's query object's list
  $search_params->list.clear();
  
  // If query is non-null, set this's query object's list to the result of parsing query
  if (!$search.empty()) {
    $search_params->$parse_query_string($search);
  }
}

std::string infra::URL::origin() const {
  if ($protocol.empty() || $host.empty()) {
    return "";
  }
  return $protocol + "//" + $host;
}

std::string infra::URL::get_protocol() const { 
  return $protocol; 
}

void infra::URL::set_protocol(const std::string &new_protocol) {
  std::string value = new_protocol;
  if (value.back() != ':') {
    value += ':';
  }
  // Basic URL parse with scheme start state as state override
  // For simplicity, just update the protocol
  $protocol = value;
  $href = $serialize_url();
}

std::string infra::URL::get_username() const { 
  return $username; 
}

void infra::URL::set_username(const std::string &new_username) {
  // If this's URL cannot have a username/password/port, then return
  if ($protocol == "file:" || $protocol == "data:" || $protocol == "about:") {
    return;
  }
  $username = new_username;
  $href = $serialize_url();
}

std::string infra::URL::get_password() const { 
  return $password; 
}

void infra::URL::set_password(const std::string &new_password) {
  // If this's URL cannot have a username/password/port, then return
  if ($protocol == "file:" || $protocol == "data:" || $protocol == "about:") {
    return;
  }
  $password = new_password;
  $href = $serialize_url();
}

std::string infra::URL::get_host() const { 
  if ($host.empty()) {
    return "";
  }
  if ($port.empty()) {
    return $host;
  }
  return $host + ":" + $port;
}

void infra::URL::set_host(const std::string &new_host) {
  // If this's URL has an opaque path, then return
  if ($pathname.find(':') != std::string::npos && $protocol != "http:" && $protocol != "https:") {
    return;
  }
  
  // Parse host and port
  size_t colon_pos = new_host.find(':');
  if (colon_pos != std::string::npos) {
    $host = new_host.substr(0, colon_pos);
    $port = new_host.substr(colon_pos + 1);
  } else {
    $host = new_host;
    // Port is not changed if not provided
  }
  $href = $serialize_url();
}

std::string infra::URL::get_hostname() const { 
  return $host.empty() ? "" : $host;
}

void infra::URL::set_hostname(const std::string &new_hostname) {
  // If this's URL has an opaque path, then return
  if ($pathname.find(':') != std::string::npos && $protocol != "http:" && $protocol != "https:") {
    return;
  }
  $host = new_hostname;
  $href = $serialize_url();
}

std::string infra::URL::get_port() const { 
  return $port; 
}

void infra::URL::set_port(const std::string &new_port) {
  // If this's URL cannot have a username/password/port, then return
  if ($protocol == "file:" || $protocol == "data:" || $protocol == "about:") {
    return;
  }
  
  if (new_port.empty()) {
    $port.clear();
  } else {
    $port = new_port;
  }
  $href = $serialize_url();
}

std::string infra::URL::get_pathname() const { 
  return $pathname; 
}

void infra::URL::set_pathname(const std::string &new_pathname) {
  // If this's URL has an opaque path, then return
  if ($pathname.find(':') != std::string::npos && $protocol != "http:" && $protocol != "https:") {
    return;
  }
  $pathname = new_pathname;
  $href = $serialize_url();
}

std::string infra::URL::get_search() const { 
  if ($search.empty()) {
    return "";
  }
  return "?" + $search;
}

void infra::URL::set_search(const std::string &new_search) {
  if (new_search.empty()) {
    $search.clear();
    $search_params->list.clear();
    $href = $serialize_url();
    return;
  }
  
  // Remove single leading '?' if any
  std::string input = new_search;
  if (!input.empty() && input[0] == '?') {
    input = input.substr(1);
  }
  
  $search = input;
  $search_params->$parse_query_string(input);
  $href = $serialize_url();
}

std::shared_ptr<infra::URLSearchParams> infra::URL::get_searchParams() {
  return $search_params;
}

std::string infra::URL::get_hash() const { 
  if ($hash.empty()) {
    return "";
  }
  return "#" + $hash;
}

void infra::URL::set_hash(const std::string &new_hash) {
  if (new_hash.empty()) {
    $hash.clear();
    $href = $serialize_url();
    return;
  }
  
  // Remove single leading '#' if any
  std::string input = new_hash;
  if (!input.empty() && input[0] == '#') {
    input = input.substr(1);
  }
  
  $hash = input;
  $href = $serialize_url();
}

std::string infra::URL::toJSON() const { 
  return $serialize_url(); 
}

bool infra::URL::$basic_url_parser(const std::string &url_string) {
  auto match = ctre::match<
      R"((?:(\w+):)?//(?:(\w+)(?::(\w+))?@)?([^:/?#]+)(?::(\d+))?([^?#]*)(?:\?([^#]*))?(?:#(.*))?)">(
      url_string);

  if (!match) {
    return false;
  }

  $protocol = match.get<1>().to_string() + ":";
  $username = match.get<2>().to_string();
  $password = match.get<3>().to_string();
  $host = match.get<4>().to_string();
  $port = match.get<5>().to_string();
  $pathname = match.get<6>().to_string();
  $search = match.get<7>().to_string();
  $hash = match.get<8>().to_string();

  $search_params = std::make_shared<URLSearchParams>();
  // Initialize search params
  $search_params->$parse_query_string($search);
  $search_params->$url_object = this;

  $href = $serialize_url();
  return true;
}

std::string infra::URL::$serialize_url() const {
  std::string result;
  if (!$protocol.empty()) {
    result += $protocol + "//";
  }
  if (!$username.empty()) {
    result += $username;
    if (!$password.empty()) {
      result += ":" + $password;
    }
    result += "@";
  }
  result += $host;
  if (!$port.empty()) {
    result += ":" + $port;
  }
  result += $pathname;
  if (!$search.empty()) {
    result += "?" + $search;
  }
  if (!$hash.empty()) {
    result += "#" + $hash;
  }
  return result;
}

void infra::URL::$parse_query_string(const std::string &query) {
  $search_params->$parse_query_string(query);
}
}; // namespace breeze::js