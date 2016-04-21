#include "utility.h"

void usageClient(const char **argv) {
    printf("Usage: %s host [port]\n", argv[0]);
    exit_(ExitCode::InvalidArguments);
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

int main(int argc, const char **argv) {
    std::pair<std::string, int> p = get_arguments(argc, argv);
    std::string host = p.first;
    int port = p.second;
    printf("Sending to host %s port: %d\n", host.c_str(), port);
    exit_(ExitCode::Ok);
}