#include "utility.h"

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

void _connect(int sockfd, const sockaddr *addr, socklen_t addrlen) {
  int err = connect(sockfd, addr, addrlen);
  if (err < 0) {
    syserr("connect");
  }
}

void _exit(ExitCode code) { exit(static_cast<int>(code)); }

void _getaddrinfo(const char *node, const char *service, const addrinfo *hints,
                  addrinfo **res) {
  int err = getaddrinfo(node, service, hints, res);
  if (err == EAI_SYSTEM) { // system error
    syserr("getaddrinfo: %s", gai_strerror(err));
  } else if (err != 0) { // other error (host not found, etc.)
    fatal("getaddrinfo: %s", gai_strerror(err));
  }
}

int _socket(int domain, int type, int protocol) {
  int result = socket(domain, type, protocol);
  if (result < 0) {
    syserr("socket");
  }
  return result;
}

void _write(int fd, const void *buf, size_t count) {
  size_t err = write(fd, buf, count);
  if (err != count) {
    syserr("partial / failed write");
  }
}

void _close(int fd) {
  int err = close(fd);
  if (err < 0) {
    syserr("close");
  }
}