#include "audio_util.h"

namespace io {

namespace audio {

namespace mp3 {

    // assumes num_bits <= 32
    // first bit is bit 0, first byte is byte 0
    uint32_t readBitsInc(uint8_t* data, int* byte, int* bit, int num_bits) {
        if (num_bits > 32) {
            //printf("NUM_BITS > 32\n");
            return -1;
        }

        if (*bit > 7) {
            //printf("BIT > 7\n");
            return -1;
        }

        uint32_t output = 0;
        while (true) {
            uint8_t ch = data[*byte];
            int bits_read;
            if (num_bits > 8) {
                bits_read = 8 - *bit;
            } else {
                bits_read = min(num_bits, 8 - *bit);
            }

            ch = ch << *bit;
            ch = ch >> (8 - bits_read);

            output = (output << bits_read) + ch;
            if (num_bits <= bits_read) {
                *bit += bits_read;
                *bit %= 8;

                *byte += bits_read / 8;

                return output;
            }

            num_bits -= bits_read;
            *bit = 0;
            (*byte)++;
        }
        return output;
    }

    uint32_t readBitsInc(uint8_t* buffer, int* offset, int count) {
        int byte = (*offset)>>3, bit = (*offset)&7;
        uint32_t result = readBitsInc(buffer, &byte, &bit, count);
        *offset += count;
        return result;
    }

    uint32_t readBits(uint8_t* buffer, int start_bit, int end_bit) {
        return readBitsInc(buffer, &start_bit, end_bit-start_bit);
    }

}

}

}
