#include "sirServer.hpp"

SirServer::SirServer(sockaddr_in server_address) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        printf("\n Socket creation error \n");
        exit(EXIT_FAILURE);
    }
    if (bind(sock, (sockaddr*)&server_address, sizeof(server_address)) < 0) {
        printf("bind failed\n");
        exit(EXIT_FAILURE);
    }
}

void SirServer::startListening() {
    uint8_t buf[64];
    while (true) {
        size_t len = 0;
        ssize_t i;
        sockaddr_in client;
        socklen_t length;
        // bytes_read = recv(sock, &buf, 64, 0);
        while (true) {
            i = recvfrom(sock, buf + len, 64 - len, 0, (sockaddr*)&client,
                         &length);
        }
        // for (i = read(sock, buf + len, 64 - len); i > 0;
        //      i = read(sock, buf + len, 64 - len)) {
        //     len += i;
        // }
        if (i < 0) {
            printf("Read error\n");
            exit(EXIT_FAILURE);
        } else {
        }
    }
}