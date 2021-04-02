#include "packet.hpp"

#include <utility>

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

void serialize_packet(Packet p, uint8_t buf[PACKET_SIZE]) {
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

Packet deserialize_packet(const uint8_t data[PACKET_SIZE]) {
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
