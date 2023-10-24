#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include "buffer.h"
#include "protocol.h"

#define MAX_READ_BYTES 1024
#define MAX_INPUT_BYTES 1024

struct Session {
  int client_socket;
  std::string nickname;
  Buffer buf;
  ServerMessage msg;

  enum class DecodeState { DecodeNick, DecodeBody, DecodeDate } decode_state;
  bool inputting;
  std::vector<std::string> buffering_msg;
};

class Client {
 public:
  Client(const std::string &server_address, int port,
         const std::string &nickname) {
    // create client socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      perror("socket");
      exit(1);
    }

    struct hostent *server = gethostbyname(server_address.c_str());

    if (server == NULL) {
      fprintf(stderr, "ERROR, no such host\n");
      exit(0);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy(server->h_addr, (char *)&server_addr.sin_addr.s_addr,
          (size_t)server->h_length);
    server_addr.sin_port = htons(port);

    session.client_socket = sock;
    session.inputting = false;
    session.nickname = nickname;
    session.decode_state = Session::DecodeState::DecodeNick;
  }

  void Run() {
    if (connect(session.client_socket, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0) {
      perror("connect() error: ");
      exit(1);
    }
    char buffer[MAX_READ_BYTES];
    char input_buffer[MAX_INPUT_BYTES];

    std::cout << "input \"m\" to input message, input \"exit\" to quit."
              << std::endl;

    while (true) {
      // clear the fd set
      FD_ZERO(&fds);

      // add stdin and client socket to the fd set
      FD_SET(STDIN_FILENO, &fds);
      FD_SET(session.client_socket, &fds);

      // wait for any of the fds to be ready for reading
      int maxfd = std::max(STDIN_FILENO, session.client_socket) + 1;
      if (select(maxfd, &fds, NULL, NULL, NULL) < 0) {
        perror("select");
        exit(1);
      }

      // check if stdin is ready for reading
      if (FD_ISSET(STDIN_FILENO, &fds)) {
        memset(&input_buffer, 0, sizeof(input_buffer));
        std::cin.getline(input_buffer, sizeof(input_buffer));
        std::string message = std::string(input_buffer);
        if (!session.inputting) {
          if (message == "m") {
            session.inputting = true;
          } else if (message == "exit") {
            break;
          }
        } else if (session.inputting) {
          // send the message to the server
          if (SendMessage(session.client_socket, message) < 0) {
            exit(1);
          }
          session.inputting = false;
          for (auto &msg : session.buffering_msg) {
            std::cout << msg << std::endl;
          }
        }
      }

      // check if client socket is ready for reading
      if (FD_ISSET(session.client_socket, &fds)) {
        // read a message from the server
        int len = read(session.client_socket, buffer, sizeof(buffer));
        if (len <= 0) {
          if (len < 0 && errno == EINTR) {
            continue;
          }
          printf("read err: len = %d\n", len);
          break;
        }
        session.buf.append(buffer, len);
        while (DecodeServerMessage(&session)) {
          ss << "{" << session.msg.date << "} "
             << "[" << session.msg.nickname << "] " << session.msg.body;
          if (session.inputting) {
            session.buffering_msg.push_back(ss.str());
          } else {
            std::cout << ss.str() << std::endl;
          }
          ss.str("");
        }
      }
    }
    close(session.client_socket);
  }

 private:
  int SendMessage(int sock, const std::string &msg) {
    ClientMessage message{session.nickname, msg};
    Buffer buf;
    message.ToBytes(buf);
    if (writen(sock, buf.peek(), buf.readable()) < 0) {
      perror("writen() error: ");
      return -1;
    }
    return 0;
  }

  bool DecodeServerMessage(Session *session) {
    auto &buf = session->buf;
    if (session->decode_state == Session::DecodeState::DecodeNick) {
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
      session->msg.nickname = std::string(buf.peek(), nickname_size);
      buf.retrieve(nickname_size);
      session->decode_state = Session::DecodeState::DecodeBody;
    }
    if (session->decode_state == Session::DecodeState::DecodeBody) {
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
      session->msg.body = std::string(buf.peek(), body_size);
      buf.retrieve(body_size);
      session->decode_state = Session::DecodeState::DecodeDate;
    }
    if (session->decode_state == Session::DecodeState::DecodeDate) {
      if (buf.readable() < 4) {
        return false;
      }
      int32_t date_size;
      ::memcpy(&date_size, buf.peek(), 4);
      date_size = be32toh(date_size);
      if (buf.readable() < 4 + date_size) {
        return false;
      }
      buf.retrieve(4);
      session->msg.date = std::string(buf.peek(), date_size);
      buf.retrieve(date_size);
      session->decode_state = Session::DecodeState::DecodeNick;
      return true;
    }
    return false;
  }

 private:
  fd_set fds;
  struct sockaddr_in server_addr;
  Session session;
  std::stringstream ss;
};

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " server_address port nickname\n";
    exit(1);
  }

  std::string server_address = argv[1];
  int port = std::stoi(argv[2]);
  std::string nickname = argv[3];

  Client c(server_address, port, nickname);
  c.Run();

  return 0;
}
