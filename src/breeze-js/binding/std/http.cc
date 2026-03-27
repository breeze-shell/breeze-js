#include "http.h"
#include <exception>
#include <stdexcept>

#include "breeze-js/quickjspp.hpp"
#include "cinatra/coro_http_client.hpp"
#include <algorithm>
#include <cctype>
#include <variant>

namespace breeze::js {

// --- Headers ---

std::string http::Headers::get(std::string name) {
  std::string lower_name = name;
  std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                 ::tolower);
  for (const auto &[k, v] : list) {
    std::string lower_k = k;
    std::transform(lower_k.begin(), lower_k.end(), lower_k.begin(), ::tolower);
    if (lower_k == lower_name)
      return v;
  }
  return "";
}

void http::Headers::set(std::string name, std::string value) {
  std::string lower_name = name;
  std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                 ::tolower);
  for (auto &[k, v] : list) {
    std::string lower_k = k;
    std::transform(lower_k.begin(), lower_k.end(), lower_k.begin(), ::tolower);
    if (lower_k == lower_name) {
      v = value;
      return;
    }
  }
  list.emplace_back(name, value);
}

bool http::Headers::has(std::string name) {
  std::string lower_name = name;
  std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                 ::tolower);
  for (const auto &[k, v] : list) {
    std::string lower_k = k;
    std::transform(lower_k.begin(), lower_k.end(), lower_k.begin(), ::tolower);
    if (lower_k == lower_name)
      return true;
  }
  return false;
}

void http::Headers::append(std::string name, std::string value) {
  list.emplace_back(name, value);
}

void http::Headers::remove_(std::string name) {
  std::string lower_name = name;
  std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                 ::tolower);
  list.erase(std::remove_if(list.begin(), list.end(),
                            [&](const auto &pair) {
                              std::string lower_k = pair.first;
                              std::transform(lower_k.begin(), lower_k.end(),
                                             lower_k.begin(), ::tolower);
                              return lower_k == lower_name;
                            }),
             list.end());
}

// --- Response ---

int http::Response::get_status() const { return $status; }

std::string http::Response::get_statusText() const { return $statusText; }

bool http::Response::get_ok() const { return $ok; }

std::string http::Response::get_url() const { return $url; }

std::shared_ptr<http::Headers> http::Response::get_headers() {
  return $headers;
}

std::string http::Response::text() {
  return std::string(reinterpret_cast<const char *>($body.data()),
                     $body.size());
}

std::vector<uint8_t> http::Response::arrayBuffer() { return $body; }

std::string http::Response::json_text() {
  // Just return text — JS side will JSON.parse
  return text();
}

JSValue http::Response::json() {
  auto *ctx = qjs::Context::current->ctx;
  auto str = text();
  JSValue result = JS_ParseJSON(ctx, str.c_str(), str.size(), "<json>");
  if (JS_IsException(result)) {
    throw qjs::exception{ctx};
  }
  return result;
}

std::shared_ptr<Blob> http::Response::blob() {
  std::shared_ptr<Blob> b = std::make_shared<Blob>();
  b->$data = $body;
  if ($headers) {
    b->$type = $headers->get("content-type");
  }
  return b;
}

// --- fetch ---

static std::string http_status_text(int status) {
  switch (status) {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 304:
    return "Not Modified";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 500:
    return "Internal Server Error";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  default:
    return "";
  }
}

async_simple::coro::Lazy<std::shared_ptr<http::Response>>
http::fetch(std::string url, std::optional<RequestInit> init) {
  cinatra::coro_http_client client;
  client.set_req_timeout(std::chrono::seconds(30));

  std::string method = "GET";
  std::string body;
  bool body_is_binary = false;
  std::unordered_map<std::string, std::string> req_headers;

  if (init.has_value()) {
    auto &opts = init.value();
    if (!opts.method.empty() && opts.method != "undefined") {
      method = opts.method;
    }
    if (opts.body.has_value()) {
      if (auto str = std::get_if<std::string>(&opts.body.value()); str) {
        body = *str;
      } else if (auto arr =
                     std::get_if<std::vector<uint8_t>>(&opts.body.value());
                 arr) {
        body = std::string(reinterpret_cast<const char *>(arr->data()),
                           arr->size());
        body_is_binary = true;
      } else if (auto blob =
                     std::get_if<std::shared_ptr<Blob>>(&opts.body.value());
                 blob) {
        body =
            std::string(reinterpret_cast<const char *>((*blob)->$data.data()),
                        (*blob)->$data.size());
        body_is_binary = true;
      } else {
        throw std::runtime_error("Unsupported body type");
      }
    }
    
    if (opts.headers.has_value()) {
      for (const auto &[k, v] : opts.headers.value()) {
        req_headers[k] = v;
      }
    }
  }

  // Apply headers using add_header for reliable HTTP formatting
  for (const auto &[k, v] : req_headers) {
    client.add_header(k, v);
  }

  cinatra::resp_data resp;

  // Determine content type from headers or body type
  cinatra::req_content_type content_type = cinatra::req_content_type::text;
  if (auto it = req_headers.find("Content-Type"); it != req_headers.end()) {
    auto &ct = it->second;
    if (ct.find("application/json") != std::string::npos)
      content_type = cinatra::req_content_type::json;
    else if (ct.find("application/x-www-form-urlencoded") != std::string::npos)
      content_type = cinatra::req_content_type::form_url_encode;
    else if (ct.find("application/octet-stream") != std::string::npos)
      content_type = cinatra::req_content_type::octet_stream;
  } else if (body_is_binary) {
    content_type = cinatra::req_content_type::octet_stream;
  }

  // Convert method string to upper case
  std::string upper_method = method;
  std::transform(upper_method.begin(), upper_method.end(), upper_method.begin(),
                 ::toupper);

  if (upper_method == "GET") {
    resp = co_await client.async_get(url);
  } else if (upper_method == "POST") {
    resp = co_await client.async_post(url, std::move(body), content_type);
  } else if (upper_method == "PUT") {
    resp = co_await client.async_put(url, std::move(body), content_type);
  } else if (upper_method == "DELETE") {
    resp = co_await client.async_delete(url, std::move(body), content_type);
  } else {
    // Fallback: use async_request with appropriate http_method
    cinatra::req_context<std::string> ctx{};
    ctx.content = std::move(body);
    cinatra::http_method hm = cinatra::http_method::GET;
    if (upper_method == "PATCH")
      hm = cinatra::http_method::PATCH;
    else if (upper_method == "HEAD")
      hm = cinatra::http_method::HEAD;
    else if (upper_method == "OPTIONS")
      hm = cinatra::http_method::OPTIONS;
    resp = co_await client.async_request(url, hm, std::move(ctx));
  }

  if (resp.net_err) {
    throw std::runtime_error("fetch failed: " + resp.net_err.message());
  }

  auto response = std::make_shared<Response>();
  response->$status = resp.status;
  response->$statusText = http_status_text(resp.status);
  response->$url = url;
  response->$ok = resp.status >= 200 && resp.status < 300;

  // Copy body
  auto body_sv = resp.resp_body;
  response->$body.assign(reinterpret_cast<const uint8_t *>(body_sv.data()),
                         reinterpret_cast<const uint8_t *>(body_sv.data()) +
                             body_sv.size());

  // Copy headers
  response->$headers = std::make_shared<Headers>();
  for (const auto &hdr : resp.resp_headers) {
    response->$headers->list.emplace_back(std::string(hdr.name),
                                          std::string(hdr.value));
  }

  co_return response;
}

} // namespace breeze::js
