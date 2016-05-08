#include "utility.h"

void usageServer(const char **argv) {
  std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
  std::cerr.flush();
  _exit(ExitCode::InvalidArguments);
}

unsigned getArguments(int argc, const char **argv) {
  if (argc > 3) {
    std::cerr << "Bad number of arguments" << std::endl;
    std::cerr.flush();
    usageServer(argv);
  }
  if (argc == 1) {
    return DEFAULT_PORT;
  }
  unsigned port = getPort(argv[1]);
  if (port == INVALID_PORT) {
    std::cerr << "Bad port number" << std::endl;
    std::cerr.flush();
    usageServer(argv);
  }
  return port;
}

Socket connectServer(unsigned port) {
  addrinfo hints;
  addrinfo *result;

  _getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result, true);

  Socket sock = 0;
  addrinfo *rp;
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    sock = _socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (_bind(sock, rp->ai_addr, rp->ai_addrlen)) {
      break;
    }

    _close(sock);
  }

  if (rp == nullptr) {
    syserr("Could not bind\n");
  }

  freeaddrinfo(result);

  makeSocketNonBlocking(sock);
  _listen(sock);
  return sock;
}

using Client = std::pair<Socket, std::string>;
using Connections = std::vector<Client>;

void removeClient(Connections &clients, Socket client) {
  _close(client); // removes from epoll
  for (auto it = clients.begin(); it != clients.end(); it++) {
    if (it->first == client) {
      clients.erase(it);
      return;
    }
  }
}

bool checkEpollError(epoll_event &event, Connections &clients) {
  if ((event.events & EPOLLERR) || (event.events & EPOLLHUP) ||
      (!(event.events & EPOLLIN))) {
    syserr("epoll");
    removeClient(clients, event.data.fd);
    return true;
  }
  return false;
}

void addClient(Connections &clients, Socket client, Epoll efd) {
  addEpollEvent(efd, client);
  clients.push_back(std::make_pair(client, std::string()));
}

void newConnectionDebug(Socket client, sockaddr in_addr, socklen_t in_len) {
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  if (getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf,
                  NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
    std::cerr << "Connection with " << client << " (host=" << hbuf
              << ", port=" << sbuf << ")" << std::endl;
    std::cerr.flush();
  }
}

bool checkListeningSocket(epoll_event &event, Socket sock, Epoll efd,
                          Connections &clients) {
  if (sock == event.data.fd) {
    while (true) {
      sockaddr in_addr;
      socklen_t in_len = sizeof in_addr;
      Socket client = _accept(sock, &in_addr, &in_len);
      if (client == -1) {
        break;
      }

#ifdef DEBUG
      newConnectionDebug(client, in_addr, in_len);
#endif // DEBUG

      makeSocketNonBlocking(client);
      addClient(clients, client, efd);
    }
    return true;
  }
  return false;
}

std::string &getNextString(Connections &clients, Socket sock) {
  static std::string empty;
  for (auto it = clients.begin(); it != clients.end(); it++) {
    if (it->first == sock) {
      return it->second;
    }
  }
  return empty;
}

std::vector<std::string> getClientData(epoll_event &event,
                                       Connections &clients) {
  std::vector<std::string> result;
  try {
    result = receiveAll(event.data.fd, getNextString(clients, event.data.fd));
  } catch (...) {
    removeClient(clients, event.data.fd);
  }
  return result;
}

void sendToOthers(const Connections &clients, const Socket sender,
                  const std::string &msg) {
  for (Client s : clients) {
    if (s.first != sender) {
      sendTo(s.first, msg);
    }
  }
}

static bool finished = false;

void cleanup(Socket sock, epoll_event *events) {
  shutdown(sock, SHUT_RDWR);
  _close(sock);
  delete[] events;
  _signal_default();
  kill(getpid(), SIGINT);
}

int main(int argc, const char **argv) {
  _signal([](int s) { finished = true; });
  unsigned port = getArguments(argc, argv);
#ifdef DEBUG
  std::cerr << "Listening on port: " << port << std::endl;
  std::cerr.flush();
#endif // DEBUG
  Socket sock = connectServer(port);
  Epoll efd = _epoll_create();
  addEpollEvent(efd, sock);

  Connections clients;
  epoll_event *events = new epoll_event[MAX_EVENTS_SERVER];

  while (!finished) {
    int numberOfEvents = epoll_wait(efd, events, MAX_EVENTS_SERVER, INFINITY);
    for (int i = 0; i < numberOfEvents; i++) {
      if (!checkEpollError(events[i], clients) &&
          !checkListeningSocket(events[i], sock, efd, clients)) {
        auto msgs = getClientData(events[i], clients);
        for (auto &msg : msgs) {
          if (msg.size() > 0) {
            sendToOthers(clients, events[i].data.fd, msg);
          }
        }
      }
    }
  }
  cleanup(sock, events);
}