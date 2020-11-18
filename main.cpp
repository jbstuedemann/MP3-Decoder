#include <bits/stdc++.h>
#include "mp3.h"

using namespace std;

void readBitsTest() {
    unsigned char* data = new unsigned char[10];
    data[0] = 0b10101101;
    data[1] = 0b01110010;
    data[2] = 0b00100110;
    data[3] = 0b00011011;

    int byte = 0;
    int bit = 0;
    int num_bits = 13;
    for (int i = 0; i + num_bits <= 32; i += num_bits) {
        printf("byte: %d\tbit: %d\t", byte, bit);
        printf("data: %d\n", io::audio::mp3::readBitsInc(data, &byte, &bit, num_bits));
    }
}

int main(){
//    readBitsTest();
//    exit(0);

    ifstream ifs;
    ifs.open ("test.mp3", std::ifstream::in);

    uint8_t data [1000];
    ifs.read((char*)data, 1000);

    // IMPORTANT: SKIP THE ID3V2
    io::audio::mp3::MP3FrameDecoder* decoder = new io::audio::mp3::MP3FrameDecoder();
    uint32_t ret_val = decoder->readFrame(data+70);

    if(decoder != nullptr){
        printf("DONE %u\n", ret_val);
    }

    return 0;
}
