#include "sir.hpp"

SirSocket::SirSocket() {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        throw system_error({}, "Cannot create socket");
    }
}

void SirSocket::ask_for_file(string server_addr, int16_t port, string path) {
    if (inet_pton(AF_INET, server_addr.c_str(), &server.sin_addr) <= 0) {
        throw invalid_argument("Invalid server address");
    };
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    file_path = path;
}

struct Packet {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint32_t seq_no;
    __uint128_t checksum;
    bool ack;
    bool nak;
    uint8_t length;
    uint8_t data[35];
};

Packet SirSocket::create_packet() {}
void SirSocket::serialize_packet(Packet p, uint8_t buf[64]) {}
