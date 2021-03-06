#ifndef SIK1_UTILITY_H
#define SIK1_UTILITY_H

#include <iostream>

#include <string>
#include <vector>

#include <csignal>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

using Socket = int;
using File = int;
using Epoll = int;

class ClosedConnectionException {};
class BadNetworkDataException {};
class EmptyMessageException {};
class MessageWithNewlineException {};

const unsigned DEFAULT_PORT = 20160;
const unsigned MIN_PORT = 1;
const unsigned MAX_PORT = (1 << 16) - 1;
const unsigned INVALID_PORT = 0;
const size_t MAX_LEN = 1000;
const size_t HEADER_LEN = 2;
const size_t BUFFER_LEN = MAX_LEN + HEADER_LEN;
const int INFINITY = -1;

const std::string INVALID_HOST = "";
const std::string INVALID_MESSAGE = "";
const File STDIN = 0;

const size_t MAX_EVENTS_CLIENT = 1500;
const size_t MAX_EVENTS_SERVER = 1500;

const uint16_t BYTE = 256;

enum class ExitCode {
  Ok = 0,
  InvalidArguments = 1,
  SystemError = 1,
  BadData = 100
};

void syserr(const char *, ...);
void _connect(Socket, const sockaddr *, socklen_t);
void _exit(ExitCode);
void _getaddrinfo(const char *, const char *, addrinfo *, addrinfo **,
                  bool = false);
Socket _socket(int, int, int);
void _close(Socket);
void _write(Socket, const void *, size_t);
unsigned getPort(const char *);
std::string getHost(const char *);
bool _bind(Socket, const sockaddr *, socklen_t);
void makeSocketNonBlocking(Socket);
void _listen(Socket);
Epoll _epoll_create();
void addEpollEvent(Epoll, Socket);
void _signal(void (*)(int));
void _signal_default();
Socket _accept(Socket, sockaddr *, socklen_t *);
void sendTo(const Socket, const std::string &);
std::vector<std::string> receiveAll(Socket, std::string &);

#endif // SIK1_UTILITY_H
