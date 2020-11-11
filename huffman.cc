#include "mp3.h"
#include "huffman_spec.h"
#include "huffman.h"

namespace io {

namespace audio {

namespace mp3 {

    uint32_t num_codes(uint32_t tn) {
        return kHuffmanTableMetadata[tn][0] * kHuffmanTableMetadata[tn][1];
    }

    HuffmanTree::HuffmanTree(uint32_t tn) : ref_count(0) {
        table_num = tn;
        linbits = kHuffmanTableMetadata[tn][2];
        root = new HuffmanTreeNode();
        for (uint32_t c = 0; c < num_codes(tn); c++) {
            HuffmanTreeNode* curr = root;
            uint32_t idx = 0;
            while (kHuffmanTableCodes[tn][c][idx] != '\0') {
                auto dig = kHuffmanTableCodes[tn][c][idx] -'0';
                if (curr->children[dig] == nullptr) {
                    curr->children[dig] = new HuffmanTreeNode();
                }
                curr = curr->children[dig];
                idx++;
            }
            curr->is_leaf = true;
            curr->sample_values[0] = kHuffmanTablePairs[tn][c][0];
            curr->sample_values[1] = kHuffmanTablePairs[tn][c][1];
        }
    }

    HuffmanTree::~HuffmanTree() {
        root = nullptr;
    }

    HuffmanTreeNode::HuffmanTreeNode() : ref_count(0), is_leaf(false) {
        children = new HuffmanTreeNode[2];
        sample_values = new int[2];
        sample_values[0] = -1;
        sample_values[1] = -1;
    }

    HuffmanTreeNode::~HuffmanTreeNode() {
        children[0] = children[1] = nullptr;
        free(children);
        free(sample_values);
    }

}

}

}
