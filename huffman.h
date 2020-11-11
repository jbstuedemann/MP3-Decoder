#ifndef INCLUDE_KERNEL_IO_HUFFMAN_H_
#define INCLUDE_KERNEL_IO_HUFFMAN_H_

#include "stdint.h"

namespace io {

namespace audio {

namespace mp3 {

    class HuffmanTreeNode {
        uint32_t ref_count;
    public:
        HuffmanTreeNode* children;
        int* sample_values;
        bool is_leaf;

        HuffmanTreeNode();
        ~HuffmanTreeNode();
    };

    class HuffmanTree {
        uint32_t ref_count;
    public:
        uint32_t table_num;
        uint32_t linbits;
        HuffmanTreeNode* root;

        HuffmanTree(uint32_t tn);
        ~HuffmanTree();
    };

}

}

}

#endif  // INCLUDE_KERNEL_IO_HUFFMAN_H_
