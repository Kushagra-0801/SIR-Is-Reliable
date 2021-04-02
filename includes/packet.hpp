#ifndef SIR_PACKET
#define SIR_PACKET

#include <arpa/inet.h>
#include <stddef.h>
#include <sys/socket.h>

using namespace std;

const size_t PACKET_SIZE = 64;
const uint8_t DATA_PREFIX[2] = {0xFE, 0xFD};

struct Packet {
    uint32_t seq_no;
    __uint128_t checksum;
    bool ack;
    bool nak;
    uint8_t length;
    uint8_t data[43];
};

void serialize_packet(Packet p, uint8_t data[PACKET_SIZE]);
Packet deserialize_packet(const uint8_t data[PACKET_SIZE]);

void copy_bytes(uint8_t* buf, const uint8_t* src, size_t n);

#endif
