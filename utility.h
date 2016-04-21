#ifndef SIK1_UTILITY_H
#define SIK1_UTILITY_H

#include <cstdio>
#include <cstdlib>

#include <string>

const int DEFAULT_PORT = 20160;
const int MIN_PORT = 1;
const int MAX_PORT = (1 << 16) - 1;
const int INVALID_PORT = -1;
const std::string INVALID_HOST = "";

const int MAX_CLIENTS = 20;

enum class ExitCode {
    Ok = 0,
    InvalidArguments = 1,
    BadData = 100
};

void exit_(ExitCode code);
int getPort(const char *cPort);
std::string getHost(const char *cHost);

#endif //SIK1_UTILITY_H
