#include "ttl.hpp"
#include <cstdio>
#include <cstring>
#include <format>
#include <memory>
#include <stdexcept>
#include "file_sink.hpp"
#include "runtime.hpp"
#include "sink.hpp"

namespace bits::ttl {

void Ttl::init(std::string_view uri) {
  constexpr const auto& p = "://";
  const auto& scheme_end  = uri.find(p);
  if (scheme_end == std::string_view::npos) {
    throw std::invalid_argument(
        std::format("invalid connection string: {}", uri));
  }

  const auto& path   = uri.substr(scheme_end + std::strlen(p));
  const auto& scheme = uri.substr(0, scheme_end);

  std::unique_ptr<ISink> sink;
  if (scheme == "file") {
    sink = std::make_unique<File>(path);
  } else if (scheme == "stdout") {
    sink = std::make_unique<StdOut>();
  } else if (scheme == "discard") {
    sink = std::make_unique<Discard>();
  } else {
    throw std::invalid_argument(std::format("unsupported scheme: {}", scheme));
  }

  auto rt = detail::Runtime::instance();
  rt->init(std::move(sink));
}

void Ttl::shutdown() {
  auto rt = detail::Runtime::instance();
  rt->shutdown();
}

}  // namespace bits::ttl
