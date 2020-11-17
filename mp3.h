#ifndef INCLUDE_KERNEL_IO_MP3_H_
#define INCLUDE_KERNEL_IO_MP3_H_

#include "stdint.h"
#include "tables.h"
#include "huffman.h"
#include <iostream>
#include <string.h>

namespace io {

namespace audio {

namespace mp3 {

    // MPEG Audio Version ID
    enum class MPEGAudioVersionId {
        kVersion2_5 = 0,
        kReserved,
        kVersion2,
        kVersion1,
    };

    // Layer Description from Frame Header
    enum class LayerDesc {
        kReserved = 0,
        kLayer3,
        kLayer2,
        kLayer1,
    };
    
    // letters correspond to letters from here: http://mpgedit.org/mpgedit/mpeg_format/mpeghdr.htm
    // IMPORTANT: reverse header bytes before copying data into this struct
    struct MP3FrameHeader {
        uint32_t emphasis : 2; // M
        uint32_t original : 1; // L
        uint32_t copyright : 1; // K
        uint32_t mode_extension : 2; // J
        uint32_t channel_mode : 2; // I
        uint32_t private_bit : 1; // H
        uint32_t padding_bit : 1; // G
        uint32_t sampling_rate_ind : 2; // F
        uint32_t bitrate_ind : 4; // E
        uint32_t protection_bit : 1;  // D
        uint32_t layer_desc : 2;  // C
        uint32_t version_id : 2;  // B
        uint32_t frame_sync : 11;  // A

        MPEGAudioVersionId getVersionId() {
            return static_cast<MPEGAudioVersionId>(version_id);
        }

        LayerDesc getLayerDesc() {
            return static_cast<LayerDesc>(layer_desc);
        }

        uint32_t getBitrate() {
            MPEGAudioVersionId version = getVersionId();
            LayerDesc layer = getLayerDesc();
            if (version == MPEGAudioVersionId::kVersion1) {
                if (layer == LayerDesc::kLayer1) {
                    return kBitRates[bitrate_ind][0]*1000;
                }
                if (layer == LayerDesc::kLayer2) {
                    return kBitRates[bitrate_ind][1]*1000;
                }
                if (layer == LayerDesc::kLayer3) {
                    return kBitRates[bitrate_ind][2]*1000;
                }
            }
            if (version == MPEGAudioVersionId::kVersion2 || version == MPEGAudioVersionId::kVersion2_5) {
                if (layer == LayerDesc::kLayer1) {
                    return kBitRates[bitrate_ind][3]*1000;
                }
                if (layer == LayerDesc::kLayer2 || layer == LayerDesc::kLayer3) {
                    return kBitRates[bitrate_ind][4]*1000;
                }
            }
            return 0xFFFFFFFF;
        }

        uint32_t getSamplingRate() {
            MPEGAudioVersionId version = getVersionId();
            if (version == MPEGAudioVersionId::kVersion1) {
                return kSamplingRates[sampling_rate_ind][0];
            }
            if (version == MPEGAudioVersionId::kVersion2) {
                return kSamplingRates[sampling_rate_ind][1];
            }
            if (version == MPEGAudioVersionId::kVersion2_5) {
                return kSamplingRates[sampling_rate_ind][2];
            }
            return 0xFFFFFFFF;
        }

        // IMPORTANT: INCLUDES HEADER LENGTH!
        uint32_t frameLength() {
            LayerDesc layer = getLayerDesc();
            if (layer == LayerDesc::kLayer1) {
                return (12 * getBitrate() / getSamplingRate() + padding_bit)*4;
            }
            if (layer == LayerDesc::kLayer2 || layer == LayerDesc::kLayer3) {
                return 144 * getBitrate() / getSamplingRate() + padding_bit;
            }
            return 0xFFFFFFFF;
        }

        void printHeader() {
            printf("*******************************\n");
            printf("************ Header ***********\n");
            printf("\temphasis: %d\n", emphasis);
            printf("\toriginal: %d\n", original);
            printf("\tcopyright: %d\n", copyright);
            printf("\tmode_extension: %d\n", mode_extension);
            printf("\tchannel_mode: %d\n", channel_mode);
            printf("\tprivate_bit: %d\n", private_bit);
            printf("\tpadding_bit: %d\n", padding_bit);
            printf("\tsampling_rate_ind: %d\n", sampling_rate_ind);
            printf("\tsampling_rate: %d\n", getSamplingRate());
            printf("\tbitrate_ind: %d\n", bitrate_ind);
            printf("\tbitrate: %d\n", getBitrate());
            printf("\tprotection_bit: %d\n", protection_bit);
            printf("\tlayer_desc: %d\n", layer_desc);
            printf("\tversion_id: %d\n", version_id);
            printf("\tframe_sync: %d\n", frame_sync);
            printf("\tframe_length: %d\n", frameLength());
        }
    } __attribute__((packed));

    // refer to pages 13, 24-30 of the link below for more information
    // https://drive.google.com/file/d/1VpsPl6ymDb4EINK42iqzqASFgaubNYDR/view
    // also useful: http://www.mp3-tech.org/programmer/docs/mp3_theory.pdf

    // THIS IS FOR FUTURE USE, MAYBE EXTENDING FOR VARIOUS BLOCK TYPES, ETC
    // block type != 2
    // struct NormalBlockInfo {
    //     uint32_t table_select : 15;
    //     uint32_t region0_count : 4;
    //     uint32_t region1_count : 3;
    // };

    // block type = 2
    // struct AbnormalBlockInfo {
    //     uint32_t block_type : 2;
    //     uint32_t mixed_block_flag : 1;
    //     uint32_t table_select : 10;
    //     uint32_t subblock_gain : 10;
    // };

    struct MP3SideInfoPrelim {
        uint32_t count1table_select_2_c2 : 1;
        uint32_t scalefac_scale_2_c2 : 1;
        uint32_t preflag_2_c2 : 1;

        uint32_t region1_count_2_c2 : 3;
        uint32_t region0_count_2_c2 : 4;
        uint32_t table_select_2_c2 : 15;

        uint32_t windows_switching_flag_2_c2 : 1;
        uint32_t scalefac_compress_2_c2 : 4;
        uint32_t global_gain_2_c2 : 8;
        uint32_t big_values_2_c2 : 9;
        uint32_t part2_3_length_2_c2 : 12;

        uint32_t count1table_select_2_c1 : 1;
        uint32_t scalefac_scale_2_c1 : 1;
        uint32_t preflag_2_c1 : 1;

        uint32_t region1_count_2_c1 : 3;
        uint32_t region0_count_2_c1 : 4;
        uint32_t table_select_2_c1 : 15;

        uint32_t windows_switching_flag_2_c1 : 1;
        uint32_t scalefac_compress_2_c1 : 4;
        uint32_t global_gain_2_c1 : 8;
        uint32_t big_values_2_c1 : 9;
        uint32_t part2_3_length_2_c1 : 12;

        uint32_t count1table_select_1_c2 : 1;
        uint32_t scalefac_scale_1_c2 : 1;
        uint32_t preflag_1_c2 : 1;

        uint32_t region1_count_1_c2 : 3;
        uint32_t region0_count_1_c2 : 4;
        uint32_t table_select_1_c2 : 15;

        uint32_t windows_switching_flag_1_c2 : 1;
        uint32_t scalefac_compress_1_c2 : 4;
        uint32_t global_gain_1_c2 : 8;
        uint32_t big_values_1_c2 : 9;
        uint32_t part2_3_length_1_c2 : 12;

        uint32_t count1table_select_1_c1 : 1;
        uint32_t scalefac_scale_1_c1 : 1;
        uint32_t preflag_1_c1 : 1;

        uint32_t region1_count_1_c1 : 3;
        uint32_t region0_count_1_c1 : 4;
        uint32_t table_select_1_c1 : 15;

        uint32_t windows_switching_flag_1_c1 : 1;
        uint32_t scalefac_compress_1_c1 : 4;
        uint32_t global_gain_1_c1 : 8;
        uint32_t big_values_1_c1 : 9;
        uint32_t part2_3_length_1_c1 : 12;

        //Side Info Meta Data
        uint32_t scsfi : 8;
        uint32_t private_bits : 3;
        uint32_t main_data_begin : 9; // if 0, main data direct

        void printSideInfo() {
            printf("*******************************\n");
            printf("********** Side Info **********\n");
            printf("\tscsfi: %d\n", scsfi);
            printf("\tprivate_bits: %d\n", private_bits);
            printf("\tmain_data_begin: %d\n", main_data_begin);
        }

        void printGranule_1() {
            printf("*******************************\n");
            printf("********** Granule 1 **********\n");
            printf("Channel 1:\n");
            printf("\tcount1table_select 1: %d\n", count1table_select_1_c1);
            printf("\tscalefac_scale 1: %d\n", scalefac_scale_1_c1);
            printf("\tpreflag 1: %d\n", preflag_1_c1);
            printf("\tregion1_count 1: %d\n", region1_count_1_c1);
            printf("\tregion0_count 1: %d\n", region0_count_1_c1);
            printf("\ttable_select 1: %d\n", table_select_1_c1);
            printf("\twindows_switching_flag 1: %d\n", windows_switching_flag_1_c1);
            printf("\tscalefac_compress 1: %d\n", scalefac_compress_1_c1);
            printf("\tglobal_gain 1: %d\n", global_gain_1_c1);
            printf("\tbig_values 1: %d\n", big_values_1_c1);
            printf("\tpart2_3_length 1: %d\n", part2_3_length_1_c1);
            printf("Channel 2:\n");
            printf("\tcount1table_select 1: %d\n", count1table_select_1_c2);
            printf("\tscalefac_scale 1: %d\n", scalefac_scale_1_c2);
            printf("\tpreflag 1: %d\n", preflag_1_c2);
            printf("\tregion1_count 1: %d\n", region1_count_1_c2);
            printf("\tregion0_count 1: %d\n", region0_count_1_c2);
            printf("\ttable_select 1: %d\n", table_select_1_c2);
            printf("\twindows_switching_flag 1: %d\n", windows_switching_flag_1_c2);
            printf("\tscalefac_compress 1: %d\n", scalefac_compress_1_c2);
            printf("\tglobal_gain 1: %d\n", global_gain_1_c2);
            printf("\tbig_values 1: %d\n", big_values_1_c2);
            printf("\tpart2_3_length 1: %d\n", part2_3_length_1_c2);
        }

        void printGranule_2() {
            printf("*******************************\n");
            printf("********** Granule 2 **********\n");
            printf("Channel 1:\n");
            printf("\tcount1table_select 2: %d\n", count1table_select_2_c1);
            printf("\tscalefac_scale 2: %d\n", scalefac_scale_2_c1);
            printf("\tpreflag 2: %d\n", preflag_2_c1);
            printf("\tregion1_count 2: %d\n", region1_count_2_c1);
            printf("\tregion0_count 2: %d\n", region0_count_2_c1);
            printf("\ttable_select 2: %d\n", table_select_2_c1);
            printf("\twindows_switching_flag 2: %d\n", windows_switching_flag_2_c1);
            printf("\tscalefac_compress 2: %d\n", scalefac_compress_2_c1);
            printf("\tglobal_gain 2: %d\n", global_gain_2_c1);
            printf("\tbig_values 2: %d\n", big_values_2_c1);
            printf("\tpart2_3_length 2: %d\n", part2_3_length_2_c1);
            printf("Channel 2:\n");
            printf("\tcount1table_select 2: %d\n", count1table_select_2_c2);
            printf("\tscalefac_scale 2: %d\n", scalefac_scale_2_c2);
            printf("\tpreflag 2: %d\n", preflag_2_c2);
            printf("\tregion1_count 2: %d\n", region1_count_2_c2);
            printf("\tregion0_count 2: %d\n", region0_count_2_c2);
            printf("\ttable_select 2: %d\n", table_select_2_c2);
            printf("\twindows_switching_flag 2: %d\n", windows_switching_flag_2_c2);
            printf("\tscalefac_compress 2: %d\n", scalefac_compress_2_c2);
            printf("\tglobal_gain 2: %d\n", global_gain_2_c2);
            printf("\tbig_values 2: %d\n", big_values_2_c2);
            printf("\tpart2_3_length 2: %d\n", part2_3_length_2_c2);
        }
    } __attribute__((packed));

    struct MP3SideInfo {
        // Metadata
        uint32_t scsfi; // 8 bits
        uint32_t private_bits; // 3 bits
        uint32_t main_data_begin; // 9 bits; if 0, main data directly follows side info, otherwise it's a negative offset from sync word

        // Actual Side info
        // field[x][y] = field for granule x + 1, channel y + 1
        uint32_t count1table_select[2][2];
        uint32_t scalefac_scale[2][2];
        uint32_t preflag[2][2];

        uint32_t region1_count[2][2];
        uint32_t region0_count[2][2];
        uint32_t table_select[2][2];

        uint32_t windows_switching_flag[2][2];
        uint32_t block_type[2][2];
        uint32_t scalefac_compress[2][2];
        uint32_t global_gain[2][2];
        uint32_t big_values[2][2];
        uint32_t part2_3_length[2][2];

        MP3SideInfo(){}
        MP3SideInfo(MP3SideInfoPrelim* side_info_prelim);
        void resetData(MP3SideInfoPrelim* side_info_prelim);

        void printSideInfo();
    };

    struct MP3FrameDecoder {
        MP3FrameHeader* header;
        MP3SideInfoPrelim* side_info_prelim;
        MP3SideInfo* side_info;

        HuffmanTree* tables [kNumHuffmanTables];

        struct {
            const unsigned *long_win;
            const unsigned *short_win;
        } band_index;
        struct {
            const unsigned *long_win;
            const unsigned *short_win;
        } band_width;

        double samples [2][2][576];
        int scalefacs [2][2][22];

        MP3FrameDecoder();
        ~MP3FrameDecoder();

        uint32_t readFrame(uint8_t* data);

        void setBandTables();
        void unpackSamples(uint8_t* main_data, int gr, int ch, int bit, int max_bit);

    };
}

}

}

#endif  // INCLUDE_KERNEL_IO_MP3_H_
