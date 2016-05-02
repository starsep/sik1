#include "utility.h"

void usageClient(const char **argv) {
  std::cerr << "Usage: " << argv[0] << " host [port]" << std::endl;
  _exit(ExitCode::InvalidArguments);
}

std::pair<std::string, unsigned> get_arguments(int argc, const char **argv) {
  if (argc < 2 || argc > 3) {
    std::cerr << "Bad number of arguments" << std::endl;
    usageClient(argv);
  }
  std::string host = getHost(argv[1]);
  if (host == INVALID_HOST) {
    std::cerr << "Bad host" << std::endl;
    usageClient(argv);
  }
  if (argc == 2) {
    return std::make_pair(host, DEFAULT_PORT);
  }
  unsigned port = getPort(argv[2]);
  if (port == INVALID_PORT) {
    std::cerr << "Bad port number" << std::endl;
    usageClient(argv);
  }
  return std::make_pair(host, port);
}

Socket connectClient(const std::string &host, unsigned port) {
  addrinfo hints;
  addrinfo *result;

  _getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);

  Socket sock =
      _socket(result->ai_family, result->ai_socktype, result->ai_protocol);

  _connect(sock, result->ai_addr, result->ai_addrlen);
  freeaddrinfo(result);

  return sock;
}

void cleanup(ExitCode exitCode) { _exit(exitCode); }

bool checkSocket(epoll_event &event, Socket sock) {
  if (event.data.fd == sock) {
    try {
      std::string msg = receive(sock);
      std::cout << msg << std::endl;
    } catch (BadNetworkDataException) {
      std::cerr << "Incorrect data received from server. Exiting." << std::endl;
      cleanup(ExitCode::BadData);
    } catch (ClosedConnectionException) {
      cleanup(ExitCode::Ok);
    }
    return true;
  }
  return false;
}

void checkStdin(Socket sock) {
  std::string msg;
  std::getline(std::cin, msg);
  sendTo(sock, msg);
}

int main(int argc, const char **argv) {
  std::pair<std::string, unsigned> p = get_arguments(argc, argv);
  std::string host = p.first;
  unsigned port = p.second;
  Socket sock = connectClient(host, port);
  makeSocketNonBlocking(sock);
  Epoll efd = _epoll_create();
  addEpollEvent(efd, sock);
  addEpollEvent(efd, STDIN);

  while (true) {
    epoll_event *events = new epoll_event[MAX_SOCKETS_CLIENT];
    int numberOfEvents = epoll_wait(efd, events, MAX_SOCKETS_CLIENT, INFINITY);
    for (int i = 0; i < numberOfEvents; i++) {
      if (!checkSocket(events[i], sock)) {
        checkStdin(sock);
      }
    }
    delete[] events;
  }
}