#pragma once
#include <helib/helib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <stdexcept>
#include <cstdint>
//inline so that server an client can have separate counts of these values
//used in order to count bytes sent and received. Can be called directly
inline uint64_t bytesSent = 0;
inline uint64_t bytesRecv = 0;
inline void writeAll(int fd, const void* buf, size_t n) {
  const char* p = (const char*)buf;
  //while still data to send
  while (n) {
    //k= amount sent
    ssize_t k = ::send(fd, p, n, 0);
    if (k <= 0) throw std::runtime_error("send failed");
    //add towrite and subtrack how much data is left
    p += k; n -= k;
    bytesSent+=k;
  }
}
inline void readAll(int fd, void* buf, size_t n) {
  char* p = (char*)buf;
  //read until nothing left to read
  while (n) {
    ssize_t k = ::recv(fd, p, n, 0);
    if (k <= 0) throw std::runtime_error("connection closed");
    p += k; n -= k;
    bytesRecv+=k;
  }
}
inline void sendBlob(int fd, const std::string& s) {
  uint64_t n = s.size();
  //send how uch to expect
  writeAll(fd, &n, sizeof n);
  //send data
  writeAll(fd, s.data(), n);
}
inline std::string recvBlob(int fd) {
  uint64_t n;
  //read how much is coming
  readAll(fd, &n, sizeof n);
  //null terminator
  std::string s(n, '\0');
  //receive data
  readAll(fd, s.data(), n);
  return s;
}

inline uint64_t sendCtxt(int fd, const helib::Ctxt& c) {
  std::stringstream ss;
  //convert ctxt to string stream for sending
  c.writeTo(ss);
  //send string stream
  sendBlob(fd, ss.str());
  //return the size of the data sent for later output
  return ss.str().size()+sizeof(uint64_t);
}
inline helib::Ctxt recvCtxt(int fd, const helib::PubKey& pk) {
  //receive a stringstream
  std::stringstream ss(recvBlob(fd));
  //use the readfrom for context
  return helib::Ctxt::readFrom(ss, pk);
}
