#include <iostream>
#include <fstream>
#include "mp3.h"

#undef M_PI

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

    printf("%f\n", io::audio::mp3::sin(io::audio::mp3::M_PI / 2));
//    readBitsTest();
    exit(0);

    ifstream ifs;
    ifs.open ("../test.mp3", std::ifstream::in);

    uint8_t data [10000];
    ifs.read((char*)data, 10000);

    auto mp3 = new io::audio::mp3::MP3(data);
    while (mp3->readNextFrame()) {
        //For now:
        break;
    }

    return 0;
}
