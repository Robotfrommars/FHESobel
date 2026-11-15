#pragma once
#include <helib/helib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <stdexcept>
#include <cstdint>

inline void writeAll(int fd, const void* buf, size_t n) {
  const char* p = (const char*)buf;
  while (n) {
    ssize_t k = ::send(fd, p, n, 0);
    if (k <= 0) throw std::runtime_error("send failed");
    p += k; n -= k;
  }
}
inline void readAll(int fd, void* buf, size_t n) {
  char* p = (char*)buf;
  while (n) {
    ssize_t k = ::recv(fd, p, n, 0);
    if (k <= 0) throw std::runtime_error("connection closed");
    p += k; n -= k;
  }
}
inline void sendBlob(int fd, const std::string& s) {
  uint64_t n = s.size();
  writeAll(fd, &n, sizeof n);
  writeAll(fd, s.data(), n);
}
inline std::string recvBlob(int fd) {
  uint64_t n;
  readAll(fd, &n, sizeof n);
  std::string s(n, '\0');
  readAll(fd, s.data(), n);
  return s;
}

inline uint64_t sendCtxt(int fd, const helib::Ctxt& c) {
  std::stringstream ss;
  c.writeTo(ss);
  sendBlob(fd, ss.str());
  return ss.str().size();
}
inline helib::Ctxt recvCtxt(int fd, const helib::PubKey& pk) {
  std::stringstream ss(recvBlob(fd));
  return helib::Ctxt::readFrom(ss, pk);
}