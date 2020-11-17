#include "mp3.h"
#include "tables.h"
#include "huffman.h"

namespace io {

namespace audio {

namespace mp3 {

    uint32_t num_codes(uint32_t tn) {
        return kHuffmanTableMetadata[tn][0] * kHuffmanTableMetadata[tn][1];
    }

    HuffmanTree::HuffmanTree(uint32_t tn) {
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

    void huffmanRecursiveDelete(HuffmanTreeNode* curr) {
        for (int i = 0; i < 2; i++) {
            if (curr->children[i] != nullptr) {
                huffmanRecursiveDelete(curr->children[i]);
            }
        }
        delete curr;
    }

    HuffmanTree::~HuffmanTree() {
        huffmanRecursiveDelete(root);
    }

    int* HuffmanTree::getSampleValues(char* buffer) {
        HuffmanTreeNode* curr = root;
        while (!curr->is_leaf) {
            curr = curr->children[buffer[0]-'0'];
            buffer++;
        }
        return curr->sample_values;
    }

    HuffmanTreeNode::HuffmanTreeNode() : is_leaf(false) {
        children = new HuffmanTreeNode*[2];
        children[0] = children[1] = nullptr; 
        sample_values = new int[2];
        sample_values[0] = sample_values[1] = -1;
    }

    HuffmanTreeNode::~HuffmanTreeNode() {
        free(children);
        free(sample_values);
    }

}

}

}
