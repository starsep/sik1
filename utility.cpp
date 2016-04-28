#include "utility.h"
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <csignal>

#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

static void syserr(const char *fmt, ...) {
  va_list fmt_args;

  fprintf(stderr, "ERROR: ");
  va_start(fmt_args, fmt);
  vfprintf(stderr, fmt, fmt_args);
  va_end(fmt_args);
  fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
  _exit(ExitCode::InvalidArguments);
}

static void fatal(const char *fmt, ...) {
  va_list fmt_args;

  fprintf(stderr, "ERROR: ");
  va_start(fmt_args, fmt);
  vfprintf(stderr, fmt, fmt_args);
  va_end(fmt_args);
  fprintf(stderr, "\n");
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

void _connect(Socket sockfd, const sockaddr *addr, socklen_t addrlen) {
  int err = connect(sockfd, addr, addrlen);
  if (err < 0) {
    syserr("connect");
  }
}

void _exit(ExitCode code) { exit(static_cast<int>(code)); }

void _getaddrinfo(const char *node, const char *service, addrinfo *hints,
                  addrinfo **res, bool passive) {
  setAddrinfo(hints, passive);
  int err = getaddrinfo(node, service, hints, res);
  if (err == EAI_SYSTEM) { // system error
    syserr("getaddrinfo: %s", gai_strerror(err));
  } else if (err != 0) { // other error (host not found, etc.)
    fatal("getaddrinfo: %s", gai_strerror(err));
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

bool _bind(Socket sockfd, const sockaddr *addr, socklen_t addrlen) {
  return bind(sockfd, addr, addrlen) == 0;
}

Debug &debug() {
  static Debug instance{};
  return instance;
}

void makeSocketNonBlocking(Socket sock) {
  int flags, s;

  flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    perror("fcntl");
  }

  flags |= O_NONBLOCK;
  s = fcntl(sock, F_SETFL, flags);
  if (s == -1) {
    perror("fcntl");
  }
}

void _listen(Socket sock) {
  if (listen(sock, SOMAXCONN)) {
    perror("listen");
  }
}

Epoll _epoll_create() {
  Epoll efd = epoll_create1(0);
  if (efd == -1) {
    perror("epoll_create");
    abort();
  }
  return efd;
}

void addEpollEvent(Epoll efd, Socket sock) {
  epoll_event event;
  event.data.fd = sock;
  event.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event) == -1) {
    perror("epoll_ctl");
    abort();
  }
}

void _signal(sighandler_t sighandler) {
  if (signal(SIGINT, sighandler)) {
    perror("signal");
  }
}

ssize_t _read(Socket sock, void *buffer, size_t maxCount) {
  ssize_t count = read(sock, buffer, maxCount);
  if (count == -1 && errno != EAGAIN) {
    perror("read");
  }
  return count;
}