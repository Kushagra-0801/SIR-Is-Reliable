#include "sir.hpp"

const uint8_t DATA_PREFIX[2] = {0xFE, 0xFD};

SirSocket::SirSocket() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw system_error({}, "Cannot create socket");
    }
    SirSocket(sock);
}

SirSocket::SirSocket(int sock_fd) {
    sock = sock_fd;
    socklen_t l;
    if (getsockname(sock, &client_ip, &l) < 0) {
        throw system_error({}, "Client IP not found");
    }
}

void SirSocket::ask_for_file(string server_addr, int16_t port, string path) {
    if (inet_pton(AF_INET, server_addr.c_str(), &server.sin_addr) <= 0) {
        throw invalid_argument("Invalid server address");
    };
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    file_path = path;

    if (file_path.size() <= 35) {
        Packet p;
        // p.src_ip = client_ip;
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

void SirSocket::serialize_packet(SirSocket::Packet p, uint8_t buf[64]) {
    uint32_t src_ip = htonl(p.src_ip);
    copy_bytes(buf, (uint8_t*)&src_ip, 4);
    uint32_t dst_ip = htonl(p.dst_ip);
    copy_bytes(buf + 4, (uint8_t*)&dst_ip, 4);
    uint32_t seq_no = htonl(p.seq_no);
    copy_bytes(buf + 8, (uint8_t*)&dst_ip, 4);

    union _u128_as_2u64 {
        __uint128_t v;
        uint64_t u[2];
    } chksm;
    chksm.v = p.checksum;
    chksm.u[0] = htonl(chksm.u[0]);
    chksm.u[1] = htonl(chksm.u[1]);
    swap(chksm.u[0], chksm.u[1]);
    copy_bytes(buf + 12, (uint8_t*)&chksm.v, 16);
    uint8_t anl = 0;
    if (p.ack) {
        anl |= (1 << 7);
    }
    if (p.nak) {
        anl |= (1 << 6);
    }
    anl |= p.length;
    copy_bytes(buf + 28, &anl, 1);
    copy_bytes(buf + 29, p.data, 35);
}
