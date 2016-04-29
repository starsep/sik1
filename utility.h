#ifndef SIK1_UTILITY_H
#define SIK1_UTILITY_H

#include "Debug.h"

#include <netdb.h>
#include <sys/epoll.h>

#include <string>
#include <vector>

using Socket = int;
using Epoll = int;

class ClosedConnectionException {};
class BadNetworkDataException {};

const int DEFAULT_PORT = 20160;
const int MIN_PORT = 1;
const int MAX_PORT = (1 << 16) - 1;
const int INVALID_PORT = -1;
const int MAX_LEN = 1000;
const int BUFFER_LEN = MAX_LEN + 2;

const std::string INVALID_HOST = "";
const Socket STDIN = 0;
const Socket STDOUT = 1;

const int MAX_CLIENT_SOCKETS = 2;
const int MAX_CLIENTS = 20;

enum class ExitCode {
  Ok = 0, InvalidArguments = 1, BadData = 100
};

void _connect(Socket, const sockaddr *, socklen_t);

void _exit(ExitCode);

void _getaddrinfo(const char *, const char *, addrinfo *, addrinfo **, bool passive = false);

Socket _socket(int, int, int);

void _close(Socket);

void _write(Socket, const void *, size_t);

int getPort(const char *);

std::string getHost(const char *);

bool _bind(Socket, const sockaddr *, socklen_t);

Debug &debug();

void makeSocketNonBlocking(Socket);

void _listen(Socket);

Epoll _epoll_create();

void addEpollEvent(Epoll efd, Socket);

void _signal(void (*)(int));

ssize_t _read(Socket, void *, size_t);

Socket _accept(Socket, sockaddr *, socklen_t *);

void sendTo(const Socket, const std::string &);

uint16_t shortFromChars(char *);

std::string receive(Socket);

#endif // SIK1_UTILITY_H
