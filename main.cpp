#include <bits/stdc++.h>
#include "mp3.h"

using namespace std;

int main(){
    ifstream ifs;
    ifs.open ("../test.mp3", std::ifstream::in);

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
