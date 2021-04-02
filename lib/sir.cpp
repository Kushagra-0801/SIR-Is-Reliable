#include "sir.hpp"

#include <openssl/md5.h>

#include <random>

SirSocket::SirSocket(int sock_fd, size_t buffer_size) {
    sock = sock_fd;
    recv_buffer.reserve(buffer_size);
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
    if (inet_pton(AF_INET, server_addr.c_str(), &peer.sin_addr) <= 0) {
        throw invalid_argument("Invalid server address");
    };
    peer.sin_port = htons(port);
    peer.sin_family = AF_INET;
    file_path = path;
    if (connect(sock, (sockaddr*)&peer, sizeof(peer)) < 0) {
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

void SirSocket::get_packet() {
    uint8_t buf[PACKET_SIZE];
    sockaddr_in addr;
    socklen_t len;
    int bytes;
    while (1) {
        bytes = recvfrom(sock, buf, PACKET_SIZE, 0, (sockaddr*)&addr, &len);
        if (bytes > 0) {
            buf[bytes] = 0;
            printf("received message: \"%s\"\n", buf);
        }
    }
}
