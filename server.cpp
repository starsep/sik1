#include "utility.h"

void usageServer(const char **argv) {
  debug() << "Usage: " << argv[0] << " [port]\n";
  _exit(ExitCode::InvalidArguments);
}

int getArguments(int argc, const char **argv) {
  if (argc > 3) {
    debug() << "Bad number of arguments\n";
    usageServer(argv);
  }
  if (argc == 1) {
    return DEFAULT_PORT;
  }
  int port = getPort(argv[1]);
  if (port == INVALID_PORT) {
    debug() << "Bad port number\n";
    usageServer(argv);
  }
  return port;
}

int connectServer(int port) {
  addrinfo hints;
  addrinfo *result;

  _getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result, true);

  int sock;
  addrinfo *rp;
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    sock = _socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (_bind(sock, rp->ai_addr, rp->ai_addrlen)) {
      break;
    }

    _close(sock);
  }

  if (rp == nullptr) {
    debug() << "Could not bind\n";
    return -1;
  }

  freeaddrinfo(result);

  return sock;
}

void removeClient(std::vector<Socket> &clients, Socket client) {
  /* Closing the descriptor will make epoll remove it
   from the set of descriptors which are monitored. */
  _close(client);
  for (auto it = clients.begin(); it != clients.end(); it++) {
    if (*it == client) {
      clients.erase(it);
      return;
    }
  }
}

bool checkEpollError(epoll_event &event, std::vector<Socket> &clients) {
  if ((event.events & EPOLLERR) || (event.events & EPOLLHUP) ||
      (!(event.events & EPOLLIN))) {
    /* An error has occured on this fd, or the socket is not
       ready for reading (why were we notified then?) */
    debug() << "epoll error\n";
    removeClient(clients, event.data.fd);
    return true;
  }
  return false;
}

void addClient(std::vector<Socket> &clients, Socket client, Epoll efd) {
  addEpollEvent(efd, client);
  clients.push_back(client);
}

void newConnectionDebug(Socket client, sockaddr in_addr, socklen_t in_len) {
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  if (getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf,
                  NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
    debug() << "Connection with " << client << " (host=" << hbuf
            << ", port=" << sbuf << ")\n";
  }
}

bool checkListeningSocket(epoll_event &event, Socket sock, Epoll efd,
                          std::vector<Socket> &clients) {
  if (sock == event.data.fd) {
    /* We have a notification on the listening socket, which
       means one or more incoming connections. */
    while (true) {
      sockaddr in_addr;
      socklen_t in_len = sizeof in_addr;
      Socket client = accept(sock, &in_addr, &in_len);
      if (client == -1) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
          /* We have processed all incoming
             connections. */
          break;
        } else {
          perror("accept");
          break;
        }
      }

      newConnectionDebug(sock, in_addr, in_len);

      /* Make the incoming socket non-blocking and add it to the
         list of fds to monitor. */
      makeSocketNonBlocking(client);
      addClient(clients, client, efd);
    }
    return true;
  }
  return false;
}

std::string getClientData(epoll_event &event, std::vector<Socket> &clients) {
  std::string result;
  char buffer[MAX_LEN];
  bool done = false;

  while (true) {
    ssize_t count = _read(event.data.fd, buffer, MAX_LEN);
    if (count == -1 && errno == EAGAIN) {
      break;
    }
    if (count == 0) {
      /* EOF, end of connection */
      done = true;
      break;
    }
    buffer[count] = '\0';
    result += buffer;
  }

  if (done) {
    debug() << "Closed connection with " << event.data.fd << '\n';
    removeClient(clients, event.data.fd);
  }
  return result;
}

void sendTo(const Socket to, const std::string &msg) {
  debug() << "Sending " << msg << " to " << to << '\n';
  _write(to, msg.c_str(), msg.size());
}

void sendToOthers(const std::vector<Socket> &clients, const Socket sender,
                  const std::string &msg) {
  for (Socket s : clients) {
    if (s != sender) {
      sendTo(s, msg);
    }
  }
}

static bool finished = false;

void signalCtrlC(int sig) { finished = true; }

void cleanup(Socket sock) {
  shutdown(sock, SHUT_RDWR);
  //  int one = 1;
  //  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
  _close(sock);
  _exit(ExitCode::Ok);
}

int main(int argc, const char **argv) {
  _signal(signalCtrlC);
  int port = getArguments(argc, argv);
  debug() << "Listening on port: " << port << "\n";
  Socket sock = connectServer(port);
  makeSocketNonBlocking(sock);
  _listen(sock);
  Epoll efd = _epoll_create();
  addEpollEvent(efd, sock);

  std::vector<Socket> clients;

  while (!finished) {
    epoll_event *events = new epoll_event[MAX_CLIENTS];
    int numberOfEvents = epoll_wait(efd, events, MAX_CLIENTS, -1);
    for (int i = 0; i < numberOfEvents; i++) {
      if (!checkEpollError(events[i], clients) &&
          !checkListeningSocket(events[i], sock, efd, clients)) {
        std::string result = getClientData(events[i], clients);
        if (result.size() > 0) {
          sendToOthers(clients, events[i].data.fd, result);
        }
      }
    }
    delete[] events;
  }
  cleanup(sock);
}