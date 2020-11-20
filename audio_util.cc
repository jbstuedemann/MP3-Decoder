#define PI    3.141592653589793
#define SQRT2 1.414213562373095

#include <cstdlib>
#include <cstdio>
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

    double sin_(double n) {
        double n_const = n;
        double n_pow = n * n * n;
        double n_fac = 2 * 3;
        int fac = 4;
        bool minus = true;
        for (int i = 0; i < 8; i++) {
            if (minus) {
                n -= n_pow / n_fac;
            } else {
                n += n_pow / n_fac;
            }

            n_pow *= n_const * n_const;
            n_fac *= (fac) * (fac + 1);
            fac += 2;
            minus = !minus;
        }

        return n;
    }

    double cos_(double n) {
        return sin_(PI / 2 - n);
    }

    double tan(double n) {
        if (sin_(n) == 0.0) {
            exit(1);
            return 0;
        }

        return sin_(n) / cos_(n);
    }

}

}

}
