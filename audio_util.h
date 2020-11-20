#ifndef INCLUDE_KERNEL_IO_AUDIO_UTIL_H_
#define INCLUDE_KERNEL_IO_AUDIO_UTIL_H_

#include "stdint.h"

namespace io {

namespace audio {

namespace mp3 {

constexpr double M_PI = 3.141592653589793;

template<typename T>
inline T min(T a, T b) {
    return (a < b ? a : b);
}

template<typename T>
inline T max(T a, T b) {
    return (a > b ? a : b);
}

uint32_t readBitsInc(uint8_t* data, int* byte, int* bit, int num_bits);

uint32_t readBitsInc(uint8_t* buffer, int* offset, int count);

uint32_t readBits(uint8_t *buffer, int start_bit, int end_bit);

double sin(double x);
double cos(double x);

}

}

}

#endif  // INCLUDE_KERNEL_IO_AUDIO_UTIL_H_
