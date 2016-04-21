#include "utility.h"

int getPort(const char *cPort) {
    std::string sPort(cPort);
    int port;
    try {
        port = std::stoi(sPort);
    } catch(...) {
        return INVALID_PORT;
    }
    return (port < MIN_PORT || port > MAX_PORT) ? INVALID_PORT : port;
}

std::string getHost(const char *cHost) {
    return std::string(cHost);
}

void exit_(ExitCode code) {
    exit(static_cast<int>(code));
}