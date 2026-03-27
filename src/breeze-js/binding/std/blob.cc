#include "blob.h"
#include <algorithm>

namespace breeze::js {

size_t Blob::get_size() const { return $data.size(); }

std::string Blob::get_type() const { return $type; }

std::vector<uint8_t> Blob::arrayBuffer() const { return $data; }

std::string Blob::text() const {
  return std::string(reinterpret_cast<const char *>($data.data()),
                     $data.size());
}

std::shared_ptr<Blob> Blob::slice(std::optional<int> start,
                                  std::optional<int> end,
                                  std::optional<std::string> contentType) {
  auto blob = std::make_shared<Blob>();

  int size = static_cast<int>($data.size());

  // Resolve start
  int s = start.value_or(0);
  if (s < 0)
    s = std::max(size + s, 0);
  else
    s = std::min(s, size);

  // Resolve end
  int e = end.value_or(size);
  if (e < 0)
    e = std::max(size + e, 0);
  else
    e = std::min(e, size);

  // Compute span
  int span = std::max(e - s, 0);

  if (span > 0) {
    blob->$data.assign($data.begin() + s, $data.begin() + s + span);
  }

  blob->$type = contentType.value_or("");
  return blob;
}

} // namespace breeze::js
