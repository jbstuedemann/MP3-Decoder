#include <iostream>
#include <fstream>
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
    freopen("output.txt", "w", stdout);
    ifstream ifs;
    ifs.open ("../test.mp3", std::ifstream::in);

    uint8_t data [4096];
    ifs.read((char*)data, 70);

    /*auto mp3 = new io::audio::mp3::MP3(data);
    uint32_t num_frames = 0;
    while (mp3->readNextFrame()) {
        num_frames++;
        break;
    }
    printf("FRAMES %u\n", num_frames);*/

    auto decoder = new io::audio::mp3::MP3FrameDecoder();
    uint32_t num_frames = 0;
    while (true) {
        ifs.read((char*)data, 4);
        if (ifs.fail()) break;
        decoder->getHeader(data);
        ifs.read((char*)(data+4), decoder->header->frameLength()-4);
        std::cout << "frame " << num_frames << ", frame len " << decoder->header->frameLength() << '\n';
        uint32_t ret = decoder->readFrame(data);
        if(ret == 0) break;
        num_frames++;
    }
    std::cout << num_frames << '\n';
    return 0;
}
