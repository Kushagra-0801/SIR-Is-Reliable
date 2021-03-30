#include "sir.hpp"

#include <openssl/md5.h>

#include <random>

const uint8_t DATA_PREFIX[2] = {0xFE, 0xFD};

SirSocket::SirSocket() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw system_error({}, "Cannot create socket");
    }
    SirSocket(sock);
}

SirSocket::SirSocket(int sock_fd) { sock = sock_fd; }

__uint128_t hex_to_u128(uint8_t* digest) {
    __uint128_t hash = 0;
    for (auto idx = 0; idx < 32; idx++) {
        if (digest[idx] >= '0' && digest[idx] <= '9') {
            hash += digest[idx] - '0';
        } else if (digest[idx] >= 'a' && digest[idx] <= 'f') {
            hash += digest[idx] - 'a';
        } else if (digest[idx] >= 'A' && digest[idx] <= 'F') {
            hash += digest[idx] - 'A';
        } else {
            exit(EXIT_FAILURE);
        }
        hash <<= 4;
    }
    return hash;
}

const size_t PACKET_SIZE = 64;

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
        uint8_t digest[MD5_DIGEST_LENGTH];
        MD5((const unsigned char*)file_path.c_str(), p.length, digest);
        p.checksum = hex_to_u128(digest);
        copy_bytes(p.data, (uint8_t*)file_path.data(), file_path.size());
        uint8_t buf[PACKET_SIZE];
        serialize_packet(p, buf);
        ssize_t len = 0;
        while (true) {
            ssize_t stat = send(sock, buf + len, PACKET_SIZE - len, 0);
            if (stat < 0) {
                exit(EXIT_FAILURE);
            } else if (stat + len == PACKET_SIZE) {
                break;
            } else {
                len += stat;
            }
        }
    }
}

/**
 * Copy Byte
 * Copies src data into the buffer
 *
 */
void copy_bytes(uint8_t* buf, uint8_t* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        buf[i] = src[i];
    }
}

void SirSocket::serialize_packet(SirSocket::Packet p,
                                 uint8_t buf[PACKET_SIZE]) {
    uint32_t seq_no = htonl(p.seq_no);
    copy_bytes(buf + 0, (uint8_t*)&seq_no, 4);

    union _u128_as_2u64 {
        __uint128_t v;
        uint64_t u[2];
    } chksm;
    chksm.v = p.checksum;
    chksm.u[0] = htonl(chksm.u[0]);
    chksm.u[1] = htonl(chksm.u[1]);
    swap(chksm.u[0], chksm.u[1]);
    copy_bytes(buf + 4, (uint8_t*)&chksm.v, 16);
    uint8_t anl = 0;
    if (p.ack) {
        anl |= (1 << 7);
    }
    if (p.nak) {
        anl |= (1 << 6);
    }
    anl |= p.length;
    copy_bytes(buf + 20, &anl, 1);
    copy_bytes(buf + 21, p.data, 43);
}
