#ifndef INCLUDE_KERNEL_IO_MP3_H_
#define INCLUDE_KERNEL_IO_MP3_H_

#include "stdint.h"
#include "tables.h"
#include "huffman.h"
#include "audio_util.h"
#include "vector.h"
#include <iostream>

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

        uint32_t channels() {
            return 2;
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
  
    struct MP3SideInfo {
        // Metadata
        bool scfsi[2][4]; // 8 bits
        int main_data_begin; // 9 bits; if 0, main data directly follows side info, otherwise it's a negative offset from sync word

        // Actual Side info
        // field[x][y] = field for granule x + 1, channel y + 1
        uint32_t part2_3_length[2][2];
        uint32_t part2_length[2][2];
        uint32_t big_value[2][2];
        uint32_t global_gain[2][2];
        uint32_t scalefac_compress[2][2];
        uint32_t slen1[2][2];
        uint32_t slen2[2][2];
        bool window_switching[2][2];
        uint32_t block_type[2][2];
        bool mixed_block_flag[2][2];
        uint32_t switch_point_l[2][2];
        uint32_t switch_point_s[2][2];
        uint32_t table_select[2][2][3];
        uint32_t subblock_gain[2][2][3];
        uint32_t region0_count[2][2];
        uint32_t region1_count[2][2];
        uint32_t preflag[2][2];
        uint32_t scalefac_scale[2][2];
        uint32_t count1table_select[2][2];

        void printSideInfo();
    };

    struct MP3FrameDecoder {
        // header and info from header
        MP3FrameHeader* header;
        struct {
            const unsigned *long_win;
            const unsigned *short_win;
        } band_index;
        struct {
            const unsigned *long_win;
            const unsigned *short_win;
        } band_width;

        // side info and info from side info
        MP3SideInfo* side_info;
        HuffmanTree* tables [kNumHuffmanTables];

        // other decoding stuffs
        int scalefac_l [2][2][22];
        int scalefac_s [2][2][3][13];

        float prev_samples [2][32][18];
        float fifo [2][1024];

        util::Vector<uint8_t> main_data_buffer;
        float samples [2][2][576];
        int16_t pcm [2304];

        static const int num_prev_frames = 9;
        int prev_frame_size [num_prev_frames];

        MP3FrameDecoder();
        ~MP3FrameDecoder();

        void getHeader(uint8_t* data);
        void postHeaderSetup();

        uint32_t readFrame(uint8_t* data);

        void setSideInfo(uint8_t* buffer);
        void setMainData(uint8_t* buffer);
        void unpackScalefacs(uint8_t* data, uint32_t granule, uint32_t channel, int &bit);
        void unpackSamples(uint8_t* main_data, int gr, int ch, int bit, int max_bit);

        void requantize(uint32_t granule, uint32_t channel);
        void midSideStereo(uint32_t granule);
        void reorder(uint32_t granule, uint32_t channel);
        void aliasReduction(uint32_t granule, uint32_t channel);
        void frequencyInversion(uint32_t granule, uint32_t channel);
        void IMDCT(uint32_t granule, uint32_t channel);
        void synthFilterbank(uint32_t granule, uint32_t channel);
        void interleave();

    };

    struct ID3 {
        uint8_t i; // == 'I'
        uint8_t d; // == 'D'
        uint8_t t; // == '3'
        uint8_t major_version;
        uint8_t revision_number;
        uint8_t flags;
        uint32_t size;
        bool isID3() {
            return (i=='I' && d=='D' && t=='3');
        }
    } __attribute__((packed));

    class MP3 {
        uint32_t current_location = 0;
        uint32_t frame_number = 0;
        MP3FrameDecoder* decoder;
        ID3* id3_tag;
        uint8_t* data;
    public:
        MP3(uint8_t* data);
        ~MP3();
        bool readNextFrame();
    };
}

}

}

#endif  // INCLUDE_KERNEL_IO_MP3_H_
