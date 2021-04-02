#ifndef SIR_SOCKET
#define SIR_SOCKET

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "packet.hpp"

using namespace std;

const size_t DEFAULT_BUFFER_SIZE = 256;

/**
 * Class to represent a particular socket.
 */
class SirSocket {
   private:
    int sock;
    sockaddr_in peer;
    string file_path;
    vector<Packet> recv_buffer;

    void get_packet();

   public:
    /**
     * Parameterised constructors
     */
    SirSocket(int sock_fd, size_t buffer_size = DEFAULT_BUFFER_SIZE);
    /**
     * Void function to request file from server.
     */
    void ask_for_file(string server_addr, int16_t port, string path);
};

#endif
