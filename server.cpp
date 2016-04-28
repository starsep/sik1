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

  setAddrinfo(&hints, true);

  _getaddrinfo(nullptr, std::to_string(port).c_str(), &hints, &result);

  int sfd;
  addrinfo *rp;
  for (rp = result; rp != nullptr; rp = rp->ai_next) {
    sfd = _socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

    if (_bind(sfd, rp->ai_addr, rp->ai_addrlen)) {
      break;
    }

    _close(sfd);
  }

  if (rp == nullptr) {
    debug() << "Could not bind\n";
    return -1;
  }

  freeaddrinfo(result);

  return sfd;
}

int main(int argc, const char **argv) {
  int port = getArguments(argc, argv);
  printf("Listening on port: %d\n", port);
  connectServer(port);
  _exit(ExitCode::Ok);
}