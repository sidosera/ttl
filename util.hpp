#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <exception>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace bits {

constexpr auto delim = ':';

inline std::optional<std::pair<std::string, uint16_t>> parseAddress(
    std::string_view address) noexcept {
  const std::size_t pos = address.rfind(delim);
  if (pos == std::string_view::npos || pos + 1 >= address.size()) {
    return std::nullopt;
  }

  const auto& host_sv = address.substr(0, pos);
  const auto& port_sv = address.substr(pos + 1);

  uint16_t port     = 0;
  const auto& first = port_sv.data();
  const auto& last  = first + port_sv.size();
  const auto& rc    = std::from_chars(first, last, port, 10);
  if (rc.ec != std::errc{} || rc.ptr != last) {
    return std::nullopt;
  }

  return std::pair<std::string, uint16_t>{std::string(host_sv), port};
}

inline int makeSockTcp() noexcept {
  return ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, IPPROTO_TCP);
}

inline int setSockOptNonBlocking(int fd) noexcept {
  const int flags = ::fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    return -1;
  }
  return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

inline int setSockOptTcpNoDelay(int fd) noexcept {
  int enable = 1;
  return ::setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
}

inline int setSockOptShared(int fd) noexcept {
  int rc     = 0;
  int enable = 1;
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) !=
      0) {
    rc = -1;
  }
  if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable)) !=
      0) {
    rc = -1;
  }

  return rc;
}

inline int setSockOptTcpKeepAlive(int fd, int idle_sec = 60,
                                  int interval_sec = 15,
                                  int probes       = 3) noexcept {
  int rc = 0;
  {
    int enable = 1;
    if (::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) !=
        0) {
      rc = -1;
    }
  }
#if __linux__
  if (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle_sec,
                   sizeof(idle_sec)) != 0) {
    rc = -1;
  }
  if (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval_sec,
                   sizeof(interval_sec)) != 0) {
    rc = -1;
  }
  if (::setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(probes)) !=
      0) {
    rc = -1;
  }
#endif
  return rc;
}

inline int sockBind(int fd, uint16_t port, std::string_view host) noexcept {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  if (::inet_pton(AF_INET, std::string(host).c_str(), &addr.sin_addr) != 1) {
    errno = EINVAL;
    return -1;
  }
  return ::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

inline int sockListen(int fd, int backlog = SOMAXCONN) noexcept {
  return ::listen(fd, backlog);
}

inline int sockConnect(int fd, uint16_t port, std::string_view host) noexcept {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  if (::inet_pton(AF_INET, std::string(host).c_str(), &addr.sin_addr) != 1) {
    errno = EINVAL;
    return -1;
  }
  return ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
}

inline std::optional<std::string> getSockOptHostPort(int fd) noexcept {
  sockaddr_in addr{};
  socklen_t len = sizeof(addr);
  if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
    return std::nullopt;
  }
  char ip[INET_ADDRSTRLEN]{};
  if (::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip)) == nullptr) {
    return std::nullopt;
  }
  const uint16_t port = ntohs(addr.sin_port);
  return std::string(ip) + delim + std::to_string(port);
}

constexpr inline uint32_t toNetwork(uint32_t v) noexcept {
  return htonl(v);
}

constexpr inline uint32_t fromNetwork(uint32_t v) noexcept {
  return ntohl(v);
}

std::array<int, 2> inline makePipe() {
  std::array<int, 2> fds{-1, -1};
  if (::pipe(fds.data()) != 0) {
    throw std::system_error(errno, std::generic_category(),
                            "failed to create pipe");
  }

  const int flags = ::fcntl(fds[0], F_GETFL, 0);
  if (flags == -1) {
    ::close(fds[0]);
    ::close(fds[1]);
    throw std::system_error(errno, std::generic_category(),
                            "failed to get pipe flags");
  }

  if (::fcntl(fds[0], F_SETFL, flags | O_NONBLOCK) == -1) {
    ::close(fds[0]);
    ::close(fds[1]);
    throw std::system_error(errno, std::generic_category(),
                            "failed to set pipe non-blocking");
  }

  return fds;
}

inline int makeEpoll() {
  const int fd = ::epoll_create1(EPOLL_CLOEXEC);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "failed to create epoll fd");
  }
  return fd;
}

}  // namespace bits
