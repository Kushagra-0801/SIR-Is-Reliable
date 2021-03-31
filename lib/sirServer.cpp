#include "sirServer.hpp"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <utility>

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

void SirServer::deal_with_client(Packet p, sockaddr_in client_addr) {
    if (p.data[0] == DATA_PREFIX[0] && p.data[1] == DATA_PREFIX[1]) {
        // TODO
    } else {
        string path{p.data, p.data + p.length};
        Packet p;
        if (filesystem::is_regular_file(path)) {
            auto size = filesystem::file_size(path);

        } else {
            for (int i = 0; i < 8; i++) {
                p.data[i] = 0xFF;
            }
        }
    }
}

void SirServer::startListening() {
    uint8_t buf[PACKET_SIZE];
    sockaddr_in client;
    ssize_t i;
    socklen_t length;
    while (true) {
        size_t len = -1;
        i = 1;
        while (i + len < 64 && i > 0) {
            len += i;
            i = recvfrom(sock, buf + len, PACKET_SIZE - len, 0,
                         (sockaddr*)&client, &length);
        }
        if (i < 0) {
            printf("Read error\n");
            exit(EXIT_FAILURE);
        } else {
            Packet p = deserialize_packet(buf);
            thread{deal_with_client, p, client}.join();
        }
    }
}
