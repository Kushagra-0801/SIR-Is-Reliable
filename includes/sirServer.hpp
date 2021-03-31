#ifndef SIR_SERVER
#define SIR_SERVER

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

using namespace std;

class Connection {};

class SirServer {
   private:
    int sock;
    vector<Connection> clients;

   public:
    SirServer(sockaddr_in server_address);
    [[noreturn]] void startListening();
};

#endif SIR_SERVER
