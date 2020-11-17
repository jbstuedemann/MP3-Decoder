
namespace io {

namespace audio {

namespace mp3 {
    inline uint32_t min(uint32_t a, uint32_t b) {
        return (a < b : a ? b);
    }

    inline uint32_t max(uint32_t a, uint32_t b) {
        return (a > b : a ? b);
    }

    // assumes num_bits <= 32
    // first bit is bit 0, first byte is byte 0
    uint32_t readBitsInc(unsigned char* data, int* byte, int* bit, int num_bits) {
        if (num_bits > 32) {
            printf("NUM_BITS > 32\n");
            exit(1);
        }

        if (*bit > 7) {
            printf("BIT > 7\n");
            exit(1);
        }

        uint32_t output = 0;
        while (true) {
            unsigned char ch = data[*byte];
            uint32_t bits_read;
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

    uint32_t getMask(uint32_t scalefac_band, uint32_t channel) {
        uint32_t mask = 0;
        if (scalefac_band < 6) {
            mask = 0b10000000;
        } else if (scalefac_band < 11) {
            mask = 0b01000000;
        } else if (scalefac_band < 16) {
            mask = 0b00100000;
        } else {
            mask = 0b00010000;
        }

        if (channel != 0) {
            mask >> 4;
        }

        return mask;
    }
}

}

}