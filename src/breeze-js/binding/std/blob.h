#pragma once
#include "../binding_helpers.h"

namespace breeze::js {
struct Blob {
  std::vector<uint8_t> $data;
  std::string $type;

  Blob() = default;

  // size of the blob in bytes
  size_t get_size() const;
  // MIME type of the blob
  std::string get_type() const;

  // Returns the blob data as an ArrayBuffer
  std::vector<uint8_t> arrayBuffer() const;
  // Returns the blob data decoded as a UTF-8 string
  std::string text() const;
  // Returns a new Blob that is a subset of this Blob
  std::shared_ptr<Blob> slice(std::optional<int> start,
                              std::optional<int> end,
                              std::optional<std::string> contentType);
};
} // namespace breeze::js
