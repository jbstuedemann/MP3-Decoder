#ifndef INCLUDE_KERNEL_IO_AUDIO_UTIL_H_
#define INCLUDE_KERNEL_IO_AUDIO_UTIL_H_

#include "stdint.h"

namespace io {

namespace audio {

namespace mp3 {

inline uint32_t min(uint32_t a, uint32_t b);

inline uint32_t max(uint32_t a, uint32_t b);

uint32_t readBitsInc(unsigned char* data, int* byte, int* bit, int num_bits);

uint32_t getMask(uint32_t scalefac_band, uint32_t channel);
    
}

}

}

#endif  // INCLUDE_KERNEL_IO_AUDIO_UTIL_H_
