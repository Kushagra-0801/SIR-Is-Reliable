#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "packet.hpp"

using namespace std;

/**
 * Class to represent a particular socket.
 */
class SirSocket {
   private:
    int sock;
    sockaddr_in server;
    string file_path;

   public:
    void serialize_packet(Packet p, uint8_t data[PACKET_SIZE]);
    Packet deserialize_packet(const uint8_t data[PACKET_SIZE]);

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
