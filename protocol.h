//
// Created by just do it on 2023/10/23.
//

#ifndef NP_AS1_PROTOCOL_H
#define NP_AS1_PROTOCOL_H
#include <endian.h>
#include <errno.h>
#include <stdint.h>
#include <algorithm>
#include <string>
#include <vector>
#include "buffer.h"

struct ClientMessage {
  std::string nickname;
  std::string body;

  void ToBytes(Buffer& buf) const {
    int32_t nick_size = htobe32(nickname.length());
    buf.append(&nick_size, 4);
    buf.append(nickname.data(), nickname.size());
    int32_t body_size = htobe32(body.length());
    buf.append(&body_size, 4);
    buf.append(body.data(), body.size());
  }
};

struct ServerMessage : public ClientMessage {
  std::string date;

  void ToBytes(Buffer& buf) const {
    ClientMessage::ToBytes(buf);
    int32_t date_size = htobe32(date.length());
    buf.append(&date_size, 4);
    buf.append(date.data(), date.size());
  }
};

ssize_t writen(int sockfd, char* buf, size_t len) {
  size_t nleft = len;
  ssize_t nwritten;
  const char* ptr = buf;

  while (nleft > 0) {
    if ((nwritten = write(sockfd, ptr, nleft)) <= 0) {
      if (nwritten < 0 && errno == EINTR) {
        nwritten = 0;
      } else {
        return -1;
      }
    }
    nleft -= nwritten;
    ptr += nwritten;
  }
  return len;
}

#endif  // NP_AS1_PROTOCOL_H
