#include "mp3.h"

namespace io {

namespace audio {

namespace mp3 {

    MP3Frame::MP3Frame(uint8_t* data) : ref_count(0) {
        header = new MP3FrameHeader{};
        side_info = new MP3SideInfo{};

        //Skip the ID3v2 for now
        data+=70;

        //Load the header data
        for(int i = 0; i < 4; i++) ((uint8_t*)header)[i] = data[3-i];
        data+=4;

        //Skip the CRC (if protection_bit is 0)
        if (!header->protection_bit)
            data+=2;

        //Load the side_info
        for(int i = 0; i < 32; i++) ((uint8_t*)side_info)[i] = data[31-i];
        for(int i = 0; i < 32; i++) printf("%02x\t",data[i]);
        printf("\n");
        //memcpy((void*)side_info, data, 32);

    }

    MP3Frame::~MP3Frame() {

    }

}

}

}
