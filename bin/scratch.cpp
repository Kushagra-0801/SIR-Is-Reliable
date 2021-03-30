// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>

// #include <iostream>
// #include <string>

// using namespace std;

// int main() {
//     int sock = socket(AF_INET, SOCK_DGRAM, 0);
//     sockaddr s;
//     socklen_t l;
//     getsockname(sock, &s, &l);
//     sockaddr_in c = *(sockaddr_in *)&s;
//     // cout << s.sa_data << " " << s.sa_family << " " << l << endl;
//     unsigned int add = c.sin_addr.s_addr;
//     cout << l << endl;
//     cout << ((add >> 24) & 0xFF) << " " << ((add >> 16) & 0xFF) << " "
//          << ((add >> 8) & 0xFF) << " " << ((add >> 0) & 0xFF) << endl;
//     return 0;
// }
