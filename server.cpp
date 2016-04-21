#include "utility.h"

void usageServer(const char **argv) {
    printf("Usage: %s [port]\n", argv[0]);
    exit(1);
}

int getArguments(int argc, const char **argv) {
    if (argc > 3) {
        printf("Bad number of arguments\n");
        usageServer(argv);
    }
    if (argc == 1) {
        return DEFAULT_PORT;
    }
    int port = getPort(argv[1]);
    if (port == INVALID_PORT) {
        printf("Bad port number\n");
        usageServer(argv);
    }
    return port;
}

int main(int argc, const char **argv) {
    int port = getArguments(argc, argv);
    printf("Listening on port: %d\n", port);
    return 0;
}