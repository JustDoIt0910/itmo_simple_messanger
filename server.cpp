#include <arpa/inet.h>
#include <endian.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "buffer.h"
#include "protocol.h"

#define MAX_READ_BYTES 1024
#define DEFAULT_MAX_CLIENT 100

struct ClientConn {
  int socket;
  Buffer buf;
  ClientMessage msg;
  enum class DecodeState { DecodeNick, DecodeBody } decode_state;

  ClientConn() : decode_state(DecodeState::DecodeNick) {}
};

class Server {
 public:
  Server(const int _port, const int max_conn = DEFAULT_MAX_CLIENT)
      : port(_port), max_client_cnt(max_conn) {
    struct sockaddr_in server_addr;

    // create server socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
      perror("socket() error: ");
      exit(1);
    }

    // bind server socket to port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
        0) {
      perror("bind() error: ");
      exit(1);
    }
  }

  void Serve() {
    // listen for incoming connections
    if (listen(listen_fd, 5) < 0) {
      perror("listen() error: ");
      exit(1);
    }

    std::cout << "Listening on port " << port << "..." << std::endl;

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    // accept and handle clients
    while (true) {
      int client_socket =
          accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
      if (client_socket < 0) {
        perror("accept() error: ");
        exit(1);
      }

      if (client_cnt.fetch_add(1) == max_client_cnt) {
        ::close(client_socket);
        client_cnt--;
        continue;
      }

      // create a new client structure
      auto client = std::make_unique<ClientConn>();
      client->socket = client_socket;
      ClientConn* client_p = client.get();

      // add the client to the list
      clients_mutex.lock();
      clients.push_back(std::move(client));
      clients_mutex.unlock();

      // create a new thread to handle the client
      std::thread t(&Server::HandleClient, this, client_p);
      t.detach();
    }
  }

 private:
  const static int kMaxReadBytes = 1024;

  int BroadcastMessage(const ServerMessage& message) {
    std::lock_guard<std::mutex> lg(clients_mutex);
    for (auto& client : clients) {
      Buffer buf;
      message.ToBytes(buf);
      if (writen(client->socket, buf.peek(), buf.readable()) < 0) {
        perror("writen() error: ");
        return -1;
      }
    }
    return 0;
  }

  bool DecodeClientMessage(ClientConn* client) {
    auto& buf = client->buf;
    if (client->decode_state == ClientConn::DecodeState::DecodeNick) {
      if (buf.readable() < 4) {
        return false;
      }
      int32_t nickname_size;
      ::memcpy(&nickname_size, buf.peek(), 4);
      nickname_size = be32toh(nickname_size);
      if (buf.readable() < 4 + nickname_size) {
        return false;
      }
      buf.retrieve(4);
      client->msg.nickname = std::string(buf.peek(), nickname_size);
      buf.retrieve(nickname_size);
      client->decode_state = ClientConn::DecodeState::DecodeBody;
    }
    if (client->decode_state == ClientConn::DecodeState::DecodeBody) {
      if (buf.readable() < 4) {
        return false;
      }
      int32_t body_size;
      ::memcpy(&body_size, buf.peek(), 4);
      body_size = be32toh(body_size);
      if (buf.readable() < 4 + body_size) {
        return false;
      }
      buf.retrieve(4);
      client->msg.body = std::string(buf.peek(), body_size);
      buf.retrieve(body_size);
      client->decode_state = ClientConn::DecodeState::DecodeNick;
      return true;
    }
    return false;
  }

  void HandleClient(ClientConn* client) {
    char buffer[kMaxReadBytes];

    while (true) {
      int len = read(client->socket, buffer, sizeof(buffer));
      if (len <= 0) {
        if (len < 0 && errno == EINTR) {
          continue;
        }
        break;
      }
      client->buf.append(buffer, len);
      while (DecodeClientMessage(client)) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_time), "%R");
        std::string time = ss.str();

        ServerMessage msg;
        msg.nickname = client->msg.nickname;
        msg.body = client->msg.body;
        msg.date = time;
        if (BroadcastMessage(msg) < 0) {
          exit(1);
        }
      }
    }

    // remove client from the list
    close(client->socket);
    std::lock_guard<std::mutex> lg(clients_mutex);
    auto it = std::find_if(
        clients.begin(), clients.end(),
        [client](auto& client_ptr) { return client_ptr.get() == client; });
    clients.erase(it);
    client_cnt--;
  }

 private:
  int listen_fd;
  int port;
  int max_client_cnt;
  std::atomic<int> client_cnt;
  std::vector<std::unique_ptr<ClientConn>> clients;
  std::mutex clients_mutex;
};

int main(int argc, char** argv) {
  if (argc != 2) {  // check the number of arguments
    std::cerr << "Usage: " << argv[0] << " port\n";
    std::cerr << argc;
    exit(1);
  }

  int port = std::stoi(argv[1]);

  Server s(port);
  s.Serve();

  return 0;
}
