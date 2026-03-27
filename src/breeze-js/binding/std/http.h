#pragma once
#include "../binding_helpers.h"
#include "blob.h"
#include <map>
#include <memory>
#include <variant>

struct JSValue;

namespace breeze::js {
struct http {

  struct Headers {
    std::vector<std::pair<std::string, std::string>> list;

    std::string get(std::string name);
    void set(std::string name, std::string value);
    bool has(std::string name);
    void append(std::string name, std::string value);
    // Note: named 'remove_' to avoid conflict with C macro 'remove'
    void remove_(std::string name);
  };

  struct Response {
    int $status;
    std::string $statusText;
    std::string $url;
    std::shared_ptr<Headers> $headers;
    std::vector<uint8_t> $body;
    bool $ok;

    int get_status() const;
    std::string get_statusText() const;
    bool get_ok() const;
    std::string get_url() const;
    std::shared_ptr<Headers> get_headers();

    // Returns body decoded as UTF-8 string
    std::string text();
    // Returns body as ArrayBuffer
    std::vector<uint8_t> arrayBuffer();
    // Returns body as JSON string (caller can JSON.parse on JS side)
    std::string json_text();
    JSValue json();
    std::shared_ptr<Blob> blob();
  };

  struct RequestInit {
    std::string method = "GET";
    // body can be a string or an ArrayBuffer (binary data)
    std::optional<
        std::variant<std::string, std::vector<uint8_t>, std::shared_ptr<Blob>>>
        body;
    // headers: accepts plain object {key: value}
    std::optional<std::map<std::string, std::string>> headers;
  };

  // Fetch a URL and return a Response
  static async_simple::coro::Lazy<std::shared_ptr<Response>>
  fetch(std::string url, std::optional<RequestInit> init);
};
} // namespace breeze::js
