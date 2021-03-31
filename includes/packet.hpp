#ifndef SIR_PACKET
#define SIR_PACKET

#include <climits.h>

using namespace std;

const size_t PACKET_SIZE = 64;

struct Packet {
    uint32_t seq_no;
    __uint128_t checksum;
    bool ack;
    bool nak;
    uint8_t length;
    uint8_t data[43];
};
#endif