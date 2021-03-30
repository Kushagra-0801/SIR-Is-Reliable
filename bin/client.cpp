// Client side C/C++ program to demonstrate Socket programming
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sir.hpp"
#define PORT 8080
#include <iostream>

using namespace std;

int main() {
    int sock = 0;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    SirSocket s{sock};
    cout << "Sent 0" << endl;
    s.ask_for_file("127.0.0.1", 8000, "Shivang and Pant for Jai-Veeru MC");
    cout << "Sent" << endl;
    return 0;
}
