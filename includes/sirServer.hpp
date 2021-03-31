#ifndef SIR_SERVER
#define SIR_SERVER

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "packet.hpp"

using namespace std;

class Connection {};

class SirServer {
   private:
    int sock;
    vector<Connection> clients;
    void deal_with_client(Packet p, sockaddr_in client_addr);

   public:
    SirServer(sockaddr_in server_address);
    [[noreturn]] void startListening();
};

#endif SIR_SERVER
