#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

using namespace std;

/**
 * Class to represent a particular socket.
 */
class SirSocket {
   private:
    int sock;
    sockaddr_in server;
    string file_path;
    struct Packet {
        uint32_t seq_no;
        __uint128_t checksum;
        bool ack;
        bool nak;
        uint8_t length;
        uint8_t data[43];
    };
    void serialize_packet(Packet p, uint8_t data[64]);

   public:
    /**
     * Default Constructor
     */
    SirSocket();
    /**
     * Parameterised constructo
     */
    SirSocket(int sock_fd);
    /**
     * Void function to request file from server.
     */
    void ask_for_file(string server_addr, int16_t port, string path);
};
