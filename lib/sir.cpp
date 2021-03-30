#include "sir.hpp"

#include <openssl/md5.h>

#include <random>

const uint8_t DATA_PREFIX[2] = {0xFE, 0xFD};

// SirSocket::SirSocket() {
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sock < 0) {
//         throw system_error({}, "Cannot create socket");
//     }
//     SirSocket(sock);
// }

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

void SirSocket::serialize_packet(Packet p, uint8_t buf[PACKET_SIZE]) {
    uint32_t seq_no = htonl(p.seq_no);
    copy_bytes(buf + 0, (uint8_t*)&seq_no, 4);

    union _u128_as_2u64 {
        __uint128_t v;
        uint32_t u[4];
    } chksm;
    chksm.v = p.checksum;
    chksm.u[0] = htonl(chksm.u[0]);
    chksm.u[1] = htonl(chksm.u[1]);
    chksm.u[2] = htonl(chksm.u[2]);
    chksm.u[3] = htonl(chksm.u[3]);
    swap(chksm.u[0], chksm.u[3]);
    swap(chksm.u[1], chksm.u[2]);
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

Packet SirSocket::deserialize_packet(const uint8_t data[PACKET_SIZE]) {
    Packet p;
    p.seq_no = ntohl(*(uint32_t*)data);
    union _u128_as_2u64 {
        __uint128_t v;
        uint32_t u[4];
    } chksm;
    chksm.u[0] = ntohl(*((uint32_t*)data + 1));
    chksm.u[1] = ntohl(*((uint32_t*)data + 2));
    chksm.u[2] = ntohl(*((uint32_t*)data + 3));
    chksm.u[3] = ntohl(*((uint32_t*)data + 4));
    p.checksum = chksm.v;
    uint8_t anl = data[20];
    p.ack = anl & (1 << 7);
    p.nak = anl & (1 << 6);
    p.length = anl & (0b00111111);
    copy_bytes(p.data, data + 21, p.length);
    return p;
}
