#include <arpa/inet.h>
#include <bits/stdc++.h>

using namespace std;

void copy_bytes(uint8_t* buf, uint8_t* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        buf[i] = src[i];
    }
}

int main() {
    uint32_t a = 0x00434241;
    uint8_t b[4];
    uint32_t c = htonl(a);
    cout << c << endl;
    copy_bytes(b, (uint8_t*)&c, 4);
    for (size_t i = 0; i < 4; i++) {
        cout << b[i] << endl;
    }
    return 0;
}