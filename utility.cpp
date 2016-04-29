#include "utility.h"
#include <csignal>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <byteswap.h>

static void syserr(const char *fmt, ...) {
  va_list fmt_args;

  fprintf(stderr, "ERROR: ");
  va_start(fmt_args, fmt);
  vfprintf(stderr, fmt, fmt_args);
  va_end(fmt_args);
  fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
  _exit(ExitCode::InvalidArguments);
}

static void setAddrinfo(addrinfo *addr, bool passive) {
  memset(addr, 0, sizeof(addrinfo));
  addr->ai_family = AF_INET; // IPv4
  addr->ai_socktype = SOCK_STREAM;
  addr->ai_protocol = IPPROTO_TCP;
  addr->ai_flags |= AI_PASSIVE & passive;
}

int getPort(const char *cPort) {
  std::string sPort(cPort);
  int port;
  try {
    port = std::stoi(sPort);
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

void _write(Socket fd, const void *buf, size_t count) {
  size_t err = write(fd, buf, count);
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

Debug &debug() {
  static Debug instance{};
  return instance;
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
  event.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event) == -1) {
    syserr("epoll_ctl");
  }
}

void _signal(sighandler_t sighandler) {
  if (signal(SIGINT, sighandler)) {
    syserr("signal");
  }
}

ssize_t _read(Socket sock, void *buffer, size_t maxCount) {
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

void sendTo(const Socket sock, const std::string &msg) {
  //debug() << "Sending " << msg << " to " << sock << '\n';
  char *buf = new char[2 + msg.size()];
  buf[0] = msg.size() / 0xff;
  buf[1] = msg.size() % 0xff;
  for (size_t i = 0; i < msg.size(); i++) {
    buf[2 + i] = msg[i];
  }
  _write(sock, buf, 2 + msg.size());
  delete[] buf;
}

uint16_t shortFromChars(char *source) {
  return source[0] * 0xff + source[1];
}

std::string receive(Socket from) {
  char buffer[BUFFER_LEN];
  while (true) {
    ssize_t count = _read(from, buffer, BUFFER_LEN);
    if (count == -1 && errno == EAGAIN) {
      break;
    }
    if (count == 0) {
      throw ClosedConnectionException();
    }
    uint16_t len = buffer[0] * 0xff + buffer[1];
    std::string result(buffer + 2, count - 2);
    if (len > MAX_LEN || result.size() != len) {
      throw BadNetworkDataException();
    }
    return result;
  }
  return "";
}