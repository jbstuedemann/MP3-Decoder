#ifndef INCLUDE_KERNEL_IO_HUFFMAN_H_
#define INCLUDE_KERNEL_IO_HUFFMAN_H_

#include "stdint.h"
#include "audio_util.h"

namespace io {

namespace audio {

namespace mp3 {

    class HuffmanTreeNode {
    public:
        HuffmanTreeNode** children;
        int* sample_values;
        bool is_leaf;

        HuffmanTreeNode();
        ~HuffmanTreeNode();
    };

    class HuffmanTree {
    public:
        uint32_t table_num;
        uint32_t linbits;
        HuffmanTreeNode* root;

        HuffmanTree(uint32_t tn);
        ~HuffmanTree();

        int* getSampleValues(uint8_t* main_data, int* bit);
    };

}

}

}

#endif  // INCLUDE_KERNEL_IO_HUFFMAN_H_
