#include "utility.h"

void usageClient(const char **argv) {
  printf("Usage: %s host [port]\n", argv[0]);
  _exit(ExitCode::InvalidArguments);
}

std::pair<std::string, int> get_arguments(int argc, const char **argv) {
  if (argc < 2 || argc > 3) {
    printf("Bad number of arguments\n");
    usageClient(argv);
  }
  std::string host = getHost(argv[1]);
  if (host == INVALID_HOST) {
    printf("Bad host\n");
    usageClient(argv);
  }
  if (argc == 2) {
    return std::make_pair(host, DEFAULT_PORT);
  }
  int port = getPort(argv[2]);
  if (port == INVALID_PORT) {
    printf("Bad port number\n");
    usageClient(argv);
  }
  return std::make_pair(host, port);
}

int connectClient(std::string host, int port) {
  addrinfo addr_hints;
  addrinfo *addr_result;

  // 'converting' host/port in string to struct addrinfo
  memset(&addr_hints, 0, sizeof(addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  _getaddrinfo(host.c_str(), std::to_string(port).c_str(), &addr_hints,
               &addr_result);

  // initialize socket according to getaddrinfo results
  int sock = _socket(addr_result->ai_family, addr_result->ai_socktype,
                     addr_result->ai_protocol);

  // connect socket to the server
  _connect(sock, addr_result->ai_addr, addr_result->ai_addrlen);
  freeaddrinfo(addr_result);

  return sock;
}

int main(int argc, const char **argv) {
  std::pair<std::string, int> p = get_arguments(argc, argv);
  std::string host = p.first;
  int port = p.second;
  printf("Sending to host %s port: %d\n", host.c_str(), port);
  int sock = connectClient(host, port);
  const char *msg = "hello world\n";
  _write(sock, msg, strlen(msg));
  _close(sock);
  _exit(ExitCode::Ok);
}