#pragma once
#include "../binding_helpers.h"
#include <functional>
#include <map> // Added for std::map
#include <memory>
#include <string>
#include <vector> // Added for std::vector

namespace breeze::js {

struct infra {
  static async_simple::coro::Lazy<void> sleep(int ms);

  // This is a blocking sleep, not recommended in JS context
  // Use sleep() instead for non-blocking sleep
  static void sleepSync(int ms);

  // Breeze uses a low resolution timer (30ms) for setTimeout and setInterval
  // due to performance concerns.
  // Use sleep() for more precise timing if needed.
  static int setTimeout(std::function<void()> callback, int ms);
  // Breeze uses a low resolution timer (30ms) for setTimeout and setInterval
  // due to performance concerns.
  // Use sleep() for more precise timing if needed.
  static int setInterval(std::function<void()> callback, int ms);
  static void clearTimeout(int id);
  static void clearInterval(int id);

  static std::string atob(std::string base64);
  static std::string btoa(std::string str);

  struct URL; // Forward declaration

  struct URLSearchParams {
    friend struct URL; // URL needs to access URLSearchParams's private members
    std::vector<std::pair<std::string, std::string>> list;
    URL *$url_object; // Pointer to the associated URL object, if any, marked
                      // with $

    URLSearchParams();
    URLSearchParams(const std::string &init);
    URLSearchParams(const std::vector<std::vector<std::string>> &init);
    URLSearchParams(const std::map<std::string, std::string> &init);

    size_t size() const;
    void append(const std::string &name, const std::string &value);
    void remove(const std::string &name, const std::string &value = "");
    std::string get(const std::string &name);
    std::vector<std::string> getAll(const std::string &name);
    bool has(const std::string &name, const std::string &value = "");
    void set(const std::string &name, const std::string &value);
    void sort();
    std::string toString() const;

  private:
    void $update_url_query();
    void $parse_query_string(const std::string &query);
  };

  struct URL {
    // Internal representation of the URL components
    std::string $href;
    std::string $protocol;
    std::string $username;
    std::string $password;
    std::string $host;
    std::string $port;
    std::string $pathname;
    std::string $search;
    std::string $hash;
    std::shared_ptr<URLSearchParams> $search_params;

    URL() = default;
    URL(const std::string &url_string);

    static std::shared_ptr<URL> parse(const std::string &url_string);
    static bool canParse(const std::string &url_string);

    std::string get_href() const;
    void set_href(const std::string &new_href);

    std::string origin() const;

    std::string get_protocol() const;
    void set_protocol(const std::string &new_protocol);

    std::string get_username() const;
    void set_username(const std::string &new_username);

    std::string get_password() const;
    void set_password(const std::string &new_password);

    std::string get_host() const;
    void set_host(const std::string &new_host);

    std::string get_hostname() const;
    void set_hostname(const std::string &new_hostname);

    std::string get_port() const;
    void set_port(const std::string &new_port);

    std::string get_pathname() const;
    void set_pathname(const std::string &new_pathname);

    std::string get_search() const;
    void set_search(const std::string &new_search);

    std::shared_ptr<URLSearchParams> get_searchParams();

    std::string get_hash() const;
    void set_hash(const std::string &new_hash);

    std::string toJSON() const;

    bool $basic_url_parser(const std::string &url_string);
    std::string $serialize_url() const;
    void $parse_query_string(const std::string &query);
  };
};

} // namespace breeze::js