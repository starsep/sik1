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

bool checkEpollError(epoll_event &event) {
  if ((event.events & EPOLLERR) || (event.events & EPOLLHUP) ||
      (!(event.events & EPOLLIN))) {
    /* An error has occured on this fd, or the socket is not
       ready for reading (why were we notified then?) */
    debug() << "epoll error\n";
    close(event.data.fd);
    return true;
  }
  return false;
}

bool checkListeningSocket(epoll_event &event, Socket sfd, Epoll efd) {
  if (sfd == event.data.fd) {
    /* We have a notification on the listening socket, which
       means one or more incoming connections. */
    while (true) {
      sockaddr in_addr;
      socklen_t in_len;
      char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

      in_len = sizeof in_addr;
      Socket infd = accept(sfd, &in_addr, &in_len);
      if (infd == -1) {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
          /* We have processed all incoming
             connections. */
          break;
        } else {
          perror("accept");
          break;
        }
      }

      int s = getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf,
                          sizeof sbuf, NI_NUMERICHOST | NI_NUMERICSERV);
      if (s == 0) {
        debug() << "Accepted connection on descriptor " << infd << " (host=" << hbuf << ", port=" << sbuf << ")\n";
      }

      /* Make the incoming socket non-blocking and add it to the
         list of fds to monitor. */
      makeSocketNonBlocking(infd);
      addEpollEvent(efd, infd);
    }
    return true;
  }
  return false;
}

void checkClientData(epoll_event &event) {
  char buffer[MAX_LEN];
  /* We have data on the fd waiting to be read. Read and
     display it. We must read whatever data is available
     completely, as we are running in edge-triggered mode
     and won't get a notification again for the same
     data. */
  bool done = false;

  while (true) {
    ssize_t count;

    count = read(event.data.fd, buffer, MAX_LEN);
    if (count == -1) {
      /* If errno == EAGAIN, that means we have read all
         data. So go back to the main loop. */
      if (errno != EAGAIN) {
        perror("read");
      }
      break;
    } else if (count == 0) {
      /* End of file. The remote has closed the
         connection. */
      done = true;
      break;
    }

    /* Write the buffer to standard output */
    _write(1, buffer, count);
  }

  if (done) {
    debug() << "Closed connection on descriptor " << event.data.fd
    << '\n';

    /* Closing the descriptor will make epoll remove it
       from the set of descriptors which are monitored. */
    close(event.data.fd);
  }
}

int main(int argc, const char **argv) {
  int port = getArguments(argc, argv);
  debug() << "Listening on port: " << port << "\n";
  Socket sfd = connectServer(port);
  makeSocketNonBlocking(sfd);
  _listen(sfd);
  Epoll efd = _epoll_create();
  addEpollEvent(efd, sfd);
  epoll_event *events = new epoll_event[MAX_CLIENTS];

  while (true) {
    int numberOfEvents = epoll_wait(efd, events, MAX_CLIENTS, -1);
//    debug() << numberOfEvents << "\n";
    for (int i = 0; i < numberOfEvents; i++) {
      if (!checkEpollError(events[i]) && !checkListeningSocket(events[i], sfd, efd)) {
        checkClientData(events[i]);
      }
    }
  }

  delete[]
          events;
  _exit(ExitCode::Ok);
}