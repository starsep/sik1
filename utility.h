#ifndef SIK1_UTILITY_H
#define SIK1_UTILITY_H

#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "Debug.h"

using Socket = int;

const int DEFAULT_PORT = 20160;
const int MIN_PORT = 1;
const int MAX_PORT = (1 << 16) - 1;
const int INVALID_PORT = -1;
const std::string INVALID_HOST = "";

const int MAX_CLIENTS = 20;

enum class ExitCode {
  Ok = 0, InvalidArguments = 1, BadData = 100
};

void _connect(Socket, const sockaddr *, socklen_t);

void _exit(ExitCode code);

void _getaddrinfo(const char *, const char *, const addrinfo *, addrinfo **);

Socket _socket(int, int, int);

void _close(Socket);

void _write(Socket, const void *, size_t);

int getPort(const char *cPort);

std::string getHost(const char *cHost);

void setAddrinfo(addrinfo *addr, bool passive = false);

bool _bind(Socket sockfd, const sockaddr *addr, socklen_t addrlen);

Debug &debug();

#endif // SIK1_UTILITY_H
