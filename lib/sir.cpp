#include "sir.hpp"

#include <openssl/md5.h>

#include <random>

SirSocket::SirSocket() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw system_error({}, "Cannot create socket");
    }
}

SirSocket::SirSocket(int sock_fd) { sock = sock_fd; }

/**
 * Copy Byte
 * Copies src data into the buffer
 *
 */
void copy_bytes(uint8_t* buf, const uint8_t* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        buf[i] = src[i];
    }
}

__uint128_t digest_to_u128(uint8_t* digest) {
    __uint128_t hash = 0;
    for (auto idx = 0; idx < MD5_DIGEST_LENGTH; idx++) {
        hash += digest[idx];
        hash <<= 8;
    }
    return hash;
}

void SirSocket::ask_for_file(string server_addr, int16_t port, string path) {
    if (inet_pton(AF_INET, server_addr.c_str(), &server.sin_addr) <= 0) {
        throw invalid_argument("Invalid server address");
    };
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    file_path = path;
    if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
        printf("\nConnection Failed \n");
        exit(EXIT_FAILURE);
    }
    if (file_path.size() <= 43) {
        Packet p;
        p.seq_no = random();
        p.ack = false;
        p.nak = false;
        p.length = file_path.size();
        uint8_t digest[MD5_DIGEST_LENGTH] = {0};
        MD5((const unsigned char*)file_path.data(), file_path.size(), digest);
        p.checksum = digest_to_u128(digest);
        copy_bytes(p.data, (uint8_t*)file_path.data(), file_path.size());
        uint8_t buf[PACKET_SIZE];
        serialize_packet(p, buf);
        ssize_t len = 0;
        while (true) {
            ssize_t stat = send(sock, buf + len, PACKET_SIZE - len, 0);
            if (stat < 0) {
                cerr << "Error in send" << endl;
                exit(EXIT_FAILURE);
            } else if (stat + len == PACKET_SIZE) {
                break;
            } else {
                len += stat;
            }
        }
    }
}
