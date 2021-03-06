#include "utility.h"

#include <csignal>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void syserr(const char *fmt, ...) {
  va_list fmt_args;

  fprintf(stderr, "ERROR: ");
  va_start(fmt_args, fmt);
  vfprintf(stderr, fmt, fmt_args);
  va_end(fmt_args);
  fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
  _exit(ExitCode::SystemError);
}

static void setAddrinfo(addrinfo *addr, bool passive) {
  memset(addr, 0, sizeof(addrinfo));
  addr->ai_family = AF_INET; // IPv4
  addr->ai_socktype = SOCK_STREAM;
  addr->ai_protocol = IPPROTO_TCP;
  addr->ai_flags |= AI_PASSIVE & passive;
}

unsigned getPort(const char *cPort) {
  std::string sPort(cPort);
  unsigned long port;
  try {
    port = std::stoul(sPort);
  } catch (...) {
    return INVALID_PORT;
  }
  return (port < MIN_PORT || port > MAX_PORT) ? INVALID_PORT : port;
}

std::string getHost(const char *cHost) { return std::string(cHost); }

void _connect(Socket sock, const sockaddr *addr, socklen_t addrlen) {
  int err = connect(sock, addr, addrlen);
  if (err < 0) {
    syserr("connect");
  }
}

void _exit(ExitCode code) { exit(static_cast<int>(code)); }

void _getaddrinfo(const char *node, const char *service, addrinfo *hints,
                  addrinfo **res, bool passive) {
  setAddrinfo(hints, passive);
  int err = getaddrinfo(node, service, hints, res);
  if (err == EAI_SYSTEM) {
    syserr("getaddrinfo: %s", gai_strerror(err));
  } else if (err != 0) {
    syserr("getaddrinfo: %s", gai_strerror(err));
  }
}

Socket _socket(int domain, int type, int protocol) {
  Socket result = socket(domain, type, protocol);
  if (result < 0) {
    syserr("socket");
  }
  return result;
}

void _write(Socket sock, const void *buf, size_t count) {
  size_t err = write(sock, buf, count);
  if (err != count) {
    syserr("partial / failed write");
  }
}

void _close(Socket fd) {
  int err = close(fd);
  if (err < 0) {
    syserr("close");
  }
}

bool _bind(Socket sock, const sockaddr *addr, socklen_t addrlen) {
  return bind(sock, addr, addrlen) == 0;
}

void makeSocketNonBlocking(Socket sock) {
  int flags, s;

  flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    syserr("fcntl");
  }

  flags |= O_NONBLOCK;
  s = fcntl(sock, F_SETFL, flags);
  if (s == -1) {
    syserr("fcntl");
  }
}

void _listen(Socket sock) {
  if (listen(sock, SOMAXCONN)) {
    syserr("listen");
  }
}

Epoll _epoll_create() {
  Epoll efd = epoll_create1(0);
  if (efd == -1) {
    syserr("epoll_create");
  }
  return efd;
}

void addEpollEvent(Epoll efd, Socket sock) {
  epoll_event event;
  event.data.fd = sock;
  event.events = EPOLLIN | EPOLLET | EPOLLHUP;
  if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event) == -1) {
    syserr("epoll_ctl");
  }
}

void _signal(sighandler_t sighandler) {
  if (signal(SIGINT, sighandler) == SIG_ERR) {
    syserr("signal");
  }
}

void _signal_default() {
  if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
    syserr("signal");
  }
}

static ssize_t _read(Socket sock, void *buffer, size_t maxCount) {
  ssize_t count = read(sock, buffer, maxCount);
  if (count == -1 && errno != EAGAIN) {
    syserr("read");
  }
  return count;
}

Socket _accept(Socket sock, sockaddr *addr, socklen_t *addrlen) {
  Socket result = accept(sock, addr, addrlen);
  if (result == -1 && !((errno == EAGAIN) || (errno == EWOULDBLOCK))) {
    syserr("accept");
  }
  return result;
}

void sendTo(const Socket sock, const std::string &m) {
  if (m.empty()) {
    return;
  }
  std::string msg = m.size() > MAX_LEN ? m.substr(0, MAX_LEN) : m;
  unsigned char *buf = new unsigned char[HEADER_LEN + msg.size()];
  buf[0] = msg.size() / BYTE;
  buf[1] = msg.size() % BYTE;
  for (size_t i = 0; i < msg.size(); i++) {
    buf[HEADER_LEN + i] = msg[i];
  }
  _write(sock, buf, HEADER_LEN + msg.size());
  delete[] buf;
}

class AllMessagesReadException {};

void receiveOne(Socket from, std::vector<std::string> &msgs, std::string &next) {
  static unsigned char buffer[BUFFER_LEN];
  std::string result = next;
  uint16_t len = 0;
  bool allMessages = false;
  for (bool first = true; true; first = false) {
    ssize_t count =
        _read(from, buffer,
              first ? BUFFER_LEN : std::min(BUFFER_LEN, len - result.size()));
    if (count == -1 && errno == EAGAIN) {
      allMessages = true;
      break;
    }
    if (count == 0) {
      throw ClosedConnectionException();
    }
    if (first) {
      len = buffer[0] * BYTE + buffer[1];
      result += std::string((char *)(buffer) + HEADER_LEN, count - HEADER_LEN);
    } else {
      result += std::string((char *)(buffer), count);
    }
    if (result.size() == len) {
      break;
    }
  }
  if (len > MAX_LEN || result.size() > len) {
    throw BadNetworkDataException();
  }
  if (result.size() < len) {
    next = result;
    return;
  }
  if (!result.empty()) {
    msgs.push_back(result);
  }
  if (allMessages) {
    throw AllMessagesReadException();
  }
  for (char c : result) {
    if (c == '\n' or c == '\0') {
      msgs.pop_back();
      throw BadNetworkDataException();
    }
  }
}

std::vector<std::string> receiveAll(Socket from, std::string &next) {
  std::vector<std::string> result;
  while (true) {
    try {
      receiveOne(from, result, next);
    } catch (AllMessagesReadException) {
      return result;
    } catch (...) {
      throw;
    }
  }
}
