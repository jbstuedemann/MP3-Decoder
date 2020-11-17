#include "mp3.h"
#include "huffman.h"

namespace io {

namespace audio {

namespace mp3 {
    uint32_t getBits(uint8_t* buffer, int start_bit, int end_bit) {
        int start_byte = 0;
        int end_byte = 0;

        start_byte = start_bit >> 3;
        end_byte = end_bit >> 3;
        start_bit = start_bit % 8;
        end_bit = end_bit % 8;

        // get the bits
        uint32_t result = ((uint32_t)buffer[start_byte] << (32 - (8 - start_bit))) >> (32 - (8 - start_bit));

        if (start_byte != end_byte) {
            while (++start_byte != end_byte) {
                result <<= 8;
                result += buffer[start_byte];
            }
            result <<= end_bit;
            result += buffer[end_byte] >> (8 - end_bit);
        } else if (end_bit != 8) {
            result >>= (8 - end_bit); 
        }

        return result;
    }

    uint32_t getBitsInc(uint8_t* buffer, int* offset, int count) {
        uint32_t result = getBits(buffer, *offset, *offset + count);
        *offset += count;
        return result;
    }

    MP3FrameDecoder::MP3FrameDecoder() {
        header = new MP3FrameHeader{};
        side_info_prelim = new MP3SideInfoPrelim{};
        side_info = new MP3SideInfo{};
        for (uint32_t i = 0; i < kNumHuffmanTables; i++) {
            tables[i] = new HuffmanTree(i);
        }
    }

    MP3FrameDecoder::~MP3FrameDecoder() {
        free(header);
        free(side_info_prelim);
        free(side_info);
        for (uint32_t i = 0; i < kNumHuffmanTables; i++) {
            free(tables[i]);
        }
    }

    // data points to the start of the frame header
    uint32_t MP3FrameDecoder::readFrame(uint8_t* data) {
        // load the header data
        for (int i = 0; i < 4; i++) {
            ((uint8_t*)header)[i] = data[3-i]; 
        }
        setBandTables();
        data += 4;

        // skip the CRC (if protection_bit is 0)
        if (!header->protection_bit) {
            data += 2;
        }

        // load the side_info
        for(int i = 0; i < 32; i++) ((uint8_t*)side_info_prelim)[i] = data[31-i];

        for(int i = 0; i < 32; i++) printf("%02x\t", data[i]);
        printf("\n");
        header->printHeader();

        side_info->resetData(side_info_prelim);

        side_info->printSideInfo();

        return header->frameLength();
    }
    
    void MP3FrameDecoder::setBandTables() {
        switch (header->getSamplingRate()) {
            case 32000:
                band_index.short_win = kBandIndexTable.short_32;
                band_width.short_win = kBandWidthTable.short_32;
                band_index.long_win = kBandIndexTable.long_32;
                band_width.long_win = kBandWidthTable.long_32;
                break;
            case 44100:
                band_index.short_win = kBandIndexTable.short_44;
                band_width.short_win = kBandWidthTable.short_44;
                band_index.long_win = kBandIndexTable.long_44;
                band_width.long_win = kBandWidthTable.long_44;
                break;
            case 48000:
                band_index.short_win = kBandIndexTable.short_48;
                band_width.short_win = kBandWidthTable.short_48;
                band_index.long_win = kBandIndexTable.long_48;
                band_width.long_win = kBandWidthTable.long_48;
                break;
        }
    }

    void MP3FrameDecoder::unpackSamples(uint8_t* main_data, int gr, int ch, int bit, int max_bit) {
        int sample = 0;
        int table_num;
        HuffmanTree* table = nullptr;

        for (int i = 0; i < 576; i++) {
            samples[gr][ch][i] = 0;
        }

        // get the big value region boundaries
        int region0;
        int region1;
        if (side_info->windows_switching_flag[gr][ch] && side_info->block_type[gr][ch] == 2) {
            region0 = 36;
            region1 = 576;
        } else {
            region0 = band_index.long_win[side_info->region0_count[gr][ch] + 1];
            region1 = band_index.long_win[side_info->region0_count[gr][ch] + 1 + side_info->region1_count[gr][ch] + 1];
        }

        // get the samples in the big value region
        // IMPORTANT: each entry in the Huffman table yields two samples
        for (; sample < (int)side_info->big_values[gr][ch] * 2; sample += 2) {
            if (sample < region0) {
                table_num = side_info->table_select[gr][ch][0];
            } else if (sample < region1) {
                table_num = side_info->table_select[gr][ch][1];
            } else {
                table_num = side_info->table_select[gr][ch][2];
            }
            table = tables[table_num];

            if (table_num == 0) {
                samples[gr][ch][sample] = 0;
                continue;
            }

            bool repeat = true;
            unsigned bit_sample = getBits(main_data, bit, bit + 32);

            // use the Huffman table and find a matching bit pattern
            for (int row = 0; row < big_value_max[table_num] && repeat; row++)
                for (int col = 0; col < big_value_max[table_num]; col++) {
                    int i = 2 * big_value_max[table_num] * row + 2 * col;
                    unsigned value = table[i];
                    unsigned size = table[i + 1];
                    if (value >> (32 - size) == bit_sample >> (32 - size)) {
                        bit += size;

                        int values[2] = {row, col};
                        for (int i = 0; i < 2; i++) {

                            // linbits extends the sample's size if needed
                            int linbit = 0;
                            if (big_value_linbit[table_num] != 0 && values[i] == big_value_max[table_num] - 1)
                                linbit = (int)getBitsInc(main_data, &bit, big_value_linbit[table_num]);

                            // if the sample is negative or positive.
                            int sign = 1;
                            if (values[i] > 0)
                                sign = getBitsInc(main_data, &bit, 1) ? -1 : 1;

                            samples[gr][ch][sample + i] = (float)(sign * (values[i] + linbit));
                        }

                        repeat = false;
                        break;
                    }
                }
        }

        // quadruples region
        for (; bit < max_bit && sample + 4 < 576; sample += 4) {
            int values[4];

            // flip bits
            if (side_info->count1table_select[gr][ch] == 1) {
                unsigned bit_sample = getBitsInc(main_data, &bit, 4);
                values[0] = (bit_sample & 0x08) > 0 ? 0 : 1;
                values[1] = (bit_sample & 0x04) > 0 ? 0 : 1;
                values[2] = (bit_sample & 0x02) > 0 ? 0 : 1;
                values[3] = (bit_sample & 0x01) > 0 ? 0 : 1;
            } else {
                unsigned bit_sample = getBits(main_data, bit, bit + 32);
                for (int entry = 0; entry < 16; entry++) {
                    unsigned value = kQuadTable.hcod[entry];
                    unsigned size = kQuadTable.hlen[entry];

                    if (value >> (32 - size) == bit_sample >> (32 - size)) {
                        bit += size;
                        for (int i = 0; i < 4; i++)
                            values[i] = (int)kQuadTable.value[entry][i];
                        break;
                    }
                }
            }

            // get the sign bit
            for (int i = 0; i < 4; i++) {
                if (values[i] > 0 && getBitsInc(main_data, &bit, 1) == 1) {
                    values[i] = -values[i];
                }
            }

            for (int i = 0; i < 4; i++) {
                samples[gr][ch][sample + i] = values[i];
            }
        }

        // fill remaining samples with zero
        for (; sample < 576; sample++) {
            samples[gr][ch][sample] = 0;
        }
    }

    MP3SideInfo::MP3SideInfo(MP3SideInfoPrelim* side_info_prelim) {
        main_data_begin = side_info_prelim->main_data_begin;
        scsfi = side_info_prelim->scsfi;
        private_bits = side_info_prelim->private_bits;
        // arrays yeet
        count1table_select[0][0] = side_info_prelim->count1table_select_1_c1;
        count1table_select[0][1] = side_info_prelim->count1table_select_1_c2;
        count1table_select[1][0] = side_info_prelim->count1table_select_2_c1;
        count1table_select[1][1] = side_info_prelim->count1table_select_2_c2;
        scalefac_scale[0][0] = side_info_prelim->scalefac_scale_1_c1;
        scalefac_scale[0][1] = side_info_prelim->scalefac_scale_1_c2;
        scalefac_scale[1][0] = side_info_prelim->scalefac_scale_2_c1;
        scalefac_scale[1][1] = side_info_prelim->scalefac_scale_2_c2;
        preflag[0][0] = side_info_prelim->preflag_1_c1;
        preflag[0][1] = side_info_prelim->preflag_1_c2;
        preflag[1][0] = side_info_prelim->preflag_2_c1;
        preflag[1][1] = side_info_prelim->preflag_2_c2;
        region1_count[0][0] = side_info_prelim->region1_count_1_c1;
        region1_count[0][1] = side_info_prelim->region1_count_1_c2;
        region1_count[1][0] = side_info_prelim->region1_count_2_c1;
        region1_count[1][1] = side_info_prelim->region1_count_2_c2;
        region0_count[0][0] = side_info_prelim->region0_count_1_c1;
        region0_count[0][1] = side_info_prelim->region0_count_1_c2;
        region0_count[1][0] = side_info_prelim->region0_count_2_c1;
        region0_count[1][1] = side_info_prelim->region0_count_2_c2;
        table_select[0][0] = side_info_prelim->table_select_1_c1;
        table_select[0][1] = side_info_prelim->table_select_1_c2;
        table_select[1][0] = side_info_prelim->table_select_2_c1;
        table_select[1][1] = side_info_prelim->table_select_2_c2;
        windows_switching_flag[0][0] = side_info_prelim->windows_switching_flag_1_c1;
        windows_switching_flag[0][1] = side_info_prelim->windows_switching_flag_1_c2;
        windows_switching_flag[1][0] = side_info_prelim->windows_switching_flag_2_c1;
        windows_switching_flag[1][1] = side_info_prelim->windows_switching_flag_2_c2;
        scalefac_compress[0][0] = side_info_prelim->scalefac_compress_1_c1;
        scalefac_compress[0][1] = side_info_prelim->scalefac_compress_1_c2;
        scalefac_compress[1][0] = side_info_prelim->scalefac_compress_2_c1;
        scalefac_compress[1][1] = side_info_prelim->scalefac_compress_2_c2;
        global_gain[0][0] = side_info_prelim->global_gain_1_c1;
        global_gain[0][1] = side_info_prelim->global_gain_1_c2;
        global_gain[1][0] = side_info_prelim->global_gain_2_c1;
        global_gain[1][1] = side_info_prelim->global_gain_2_c2;
        big_values[0][0] = side_info_prelim->big_values_1_c1;
        big_values[0][1] = side_info_prelim->big_values_1_c2;
        big_values[1][0] = side_info_prelim->big_values_2_c1;
        big_values[1][1] = side_info_prelim->big_values_2_c2;
        part2_3_length[0][0] = side_info_prelim->part2_3_length_1_c1;
        part2_3_length[0][1] = side_info_prelim->part2_3_length_1_c2;
        part2_3_length[1][0] = side_info_prelim->part2_3_length_2_c1;
        part2_3_length[1][1] = side_info_prelim->part2_3_length_2_c2;
    }

    void MP3SideInfo::resetData(MP3SideInfoPrelim* side_info_prelim) {
        main_data_begin = side_info_prelim->main_data_begin;
        scsfi = side_info_prelim->scsfi;
        private_bits = side_info_prelim->private_bits;
        // arrays yeet
        count1table_select[0][0] = side_info_prelim->count1table_select_1_c1;
        count1table_select[0][1] = side_info_prelim->count1table_select_1_c2;
        count1table_select[1][0] = side_info_prelim->count1table_select_2_c1;
        count1table_select[1][1] = side_info_prelim->count1table_select_2_c2;
        scalefac_scale[0][0] = side_info_prelim->scalefac_scale_1_c1;
        scalefac_scale[0][1] = side_info_prelim->scalefac_scale_1_c2;
        scalefac_scale[1][0] = side_info_prelim->scalefac_scale_2_c1;
        scalefac_scale[1][1] = side_info_prelim->scalefac_scale_2_c2;
        preflag[0][0] = side_info_prelim->preflag_1_c1;
        preflag[0][1] = side_info_prelim->preflag_1_c2;
        preflag[1][0] = side_info_prelim->preflag_2_c1;
        preflag[1][1] = side_info_prelim->preflag_2_c2;
        region1_count[0][0] = side_info_prelim->region1_count_1_c1;
        region1_count[0][1] = side_info_prelim->region1_count_1_c2;
        region1_count[1][0] = side_info_prelim->region1_count_2_c1;
        region1_count[1][1] = side_info_prelim->region1_count_2_c2;
        region0_count[0][0] = side_info_prelim->region0_count_1_c1;
        region0_count[0][1] = side_info_prelim->region0_count_1_c2;
        region0_count[1][0] = side_info_prelim->region0_count_2_c1;
        region0_count[1][1] = side_info_prelim->region0_count_2_c2;
        table_select[0][0] = side_info_prelim->table_select_1_c1;
        table_select[0][1] = side_info_prelim->table_select_1_c2;
        table_select[1][0] = side_info_prelim->table_select_2_c1;
        table_select[1][1] = side_info_prelim->table_select_2_c2;
        windows_switching_flag[0][0] = side_info_prelim->windows_switching_flag_1_c1;
        windows_switching_flag[0][1] = side_info_prelim->windows_switching_flag_1_c2;
        windows_switching_flag[1][0] = side_info_prelim->windows_switching_flag_2_c1;
        windows_switching_flag[1][1] = side_info_prelim->windows_switching_flag_2_c2;
        scalefac_compress[0][0] = side_info_prelim->scalefac_compress_1_c1;
        scalefac_compress[0][1] = side_info_prelim->scalefac_compress_1_c2;
        scalefac_compress[1][0] = side_info_prelim->scalefac_compress_2_c1;
        scalefac_compress[1][1] = side_info_prelim->scalefac_compress_2_c2;
        global_gain[0][0] = side_info_prelim->global_gain_1_c1;
        global_gain[0][1] = side_info_prelim->global_gain_1_c2;
        global_gain[1][0] = side_info_prelim->global_gain_2_c1;
        global_gain[1][1] = side_info_prelim->global_gain_2_c2;
        big_values[0][0] = side_info_prelim->big_values_1_c1;
        big_values[0][1] = side_info_prelim->big_values_1_c2;
        big_values[1][0] = side_info_prelim->big_values_2_c1;
        big_values[1][1] = side_info_prelim->big_values_2_c2;
        part2_3_length[0][0] = side_info_prelim->part2_3_length_1_c1;
        part2_3_length[0][1] = side_info_prelim->part2_3_length_1_c2;
        part2_3_length[1][0] = side_info_prelim->part2_3_length_2_c1;
        part2_3_length[1][1] = side_info_prelim->part2_3_length_2_c2;
    }

    void MP3SideInfo::printSideInfo() {
        printf("main_data_begin: %d\n", main_data_begin);
        printf("private_bits: %d\n", private_bits);
        printf("scsfi: %d\n", scsfi);

        for (int i = 0; i < 2; i++) {
            printf("************ GRANULE %d ***********\n", i + 1);
            for (int j = 0; j < 2; j++) {
                printf("************ CHANNEL %d ***********\n", j + 1);
                printf("count1table_select: %d\n", count1table_select[i][j]);
                printf("scalefac_scale: %d\n", scalefac_scale[i][j]);
                printf("preflag: %d\n", preflag[i][j]);
                printf("region1_count: %d\n", region1_count[i][j]);
                printf("region0_count: %d\n", region0_count[i][j]);
                printf("table_select: %d\n", table_select[i][j]);
                printf("windows_switching_flag: %d\n", windows_switching_flag[i][j]);
                printf("scalefac_compress: %d\n", scalefac_compress[i][j]);
                printf("global_gain: %d\n", global_gain[i][j]);
                printf("big_values: %d\n", big_values[i][j]);
                printf("part2_3_length: %d\n", part2_3_length[i][j]);
            }
        }
    }

}

}

}
