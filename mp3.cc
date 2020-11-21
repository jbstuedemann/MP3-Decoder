#include "mp3.h"
#include "math.h"

namespace io {

namespace audio {

namespace mp3 {
    
    MP3FrameDecoder::MP3FrameDecoder() {
        header = new MP3FrameHeader{};
        side_info = new MP3SideInfo{};
        for (uint32_t i = 0; i < kNumHuffmanTables; i++) {
            tables[i] = new HuffmanTree(i);
        }
    }

    MP3FrameDecoder::~MP3FrameDecoder() {
        free(header);
        free(side_info);
        for (uint32_t i = 0; i < kNumHuffmanTables; i++) {
            free(tables[i]);
        }
    }

    void MP3FrameDecoder::postHeaderSetup() {
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
        for (int i = num_prev_frames-1; i > 0; i--) {
            prev_frame_size[i] = prev_frame_size[i - 1];
        }
        prev_frame_size[0] = header->frameLength();
    }

    void MP3FrameDecoder::getHeader(uint8_t* data) {
        // load the header data
        for (int i = 0; i < 4; i++) {
            ((uint8_t*)header)[i] = data[3-i];
        }
        postHeaderSetup();
    }

    // data points to the start of the frame header
    uint32_t MP3FrameDecoder::readFrame(uint8_t* data) {
        // store start of frame
        uint8_t* frame_start = data;
        data += 4;

        // if the frame sync is not all 1s, this is not an MP3 frame
        if (header->frame_sync != 2047) return 0;

        // skip the CRC (if protection_bit is 0)
        if (!header->protection_bit) {
            data += 2;
        }

        // process side info and main data
        setSideInfo(data);
        setMainData(frame_start);

        //header->printHeader();
        //side_info->printSideInfo();

        // produce samples
        for (int gr = 0; gr < 2; gr++) {
            for (uint32_t ch = 0; ch < header->channels(); ch++) {
                requantize(gr, ch);
            }

            if (header->channel_mode == 1 && (header->mode_extension >> 1)) {
                midSideStereo(gr);
            }

            for (uint32_t ch = 0; ch < header->channels(); ch++) {
                if (side_info->block_type[gr][ch] == 2 || side_info->mixed_block_flag[gr][ch]) {
                    reorder(gr, ch);
                } else {
                    aliasReduction(gr, ch);
                }
                IMDCT(gr, ch);
                frequencyInversion(gr, ch);
                synthFilterbank(gr, ch);
            }
        }

        interleave();

        return header->frameLength();
    }

    void MP3FrameDecoder::setMainData(uint8_t* buffer) {
        int constant = 36+2*(header->protection_bit == 0);
        // put the main data in a separate buffer so that side info and header
        // do not interfere; main_data_begin may be larger than the previous frame
        // and does not include the size of side info and headers
        uint32_t frame_size = header->frameLength();
        if (side_info->main_data_begin == 0) {
            main_data_buffer.setSize(frame_size - constant);
            memcpy(&main_data_buffer[0], buffer + constant, frame_size - constant);
        } else {
            int bound = 0;
            for (int frame = 0; frame < num_prev_frames; frame++) {
                bound += prev_frame_size[frame] - constant;
                if ((int)side_info->main_data_begin < bound) {
                    int ptr_offset = side_info->main_data_begin + frame * constant;
                    int buffer_offset = 0;

                    int part[num_prev_frames];
                    part[frame] = side_info->main_data_begin;
                    for (int i = 0; i <= frame-1; i++) {
                        part[i] = prev_frame_size[i] - constant;
                        part[frame] -= part[i];
                    }

                    main_data_buffer.setSize(frame_size - constant + side_info->main_data_begin);
                    memcpy(main_data_buffer.data(), buffer - ptr_offset, part[frame]);
                    ptr_offset -= (part[frame] + constant);
                    buffer_offset += part[frame];
                    for (int i = frame-1; i >= 0; i--) {
                        memcpy(&main_data_buffer[buffer_offset], buffer - ptr_offset, part[i]);
                        ptr_offset -= (part[i] + constant);
                        buffer_offset += part[i];
                    }
                    memcpy(&main_data_buffer[side_info->main_data_begin], buffer + constant, frame_size - constant);
                    break;
                }
            }
        }

        int bit = 0;
        for (int gr = 0; gr < 2; gr++)
            for (uint32_t ch = 0; ch < header->channels(); ch++) {
                int max_bit = bit + side_info->part2_3_length[gr][ch];
                unpackScalefacs(main_data_buffer.data(), gr, ch, bit);
                unpackSamples(main_data_buffer.data(), gr, ch, bit, max_bit);
                bit = max_bit;
            }
    }

    void MP3FrameDecoder::setSideInfo(uint8_t* buffer) {
        int count = 0;

        // number of bytes the main data ends before the next frame header
        side_info->main_data_begin = (int)readBitsInc(buffer, &count, 9);

        // skip private bits
        count += 3; 

        for (uint32_t ch = 0; ch < header->channels(); ch++)
            for (int scfsi_band = 0; scfsi_band < 4; scfsi_band++)
                // scale factor selection information.
                // if scfsi[scfsi_band] == 1, then scale factors for the first granule are reused in the second granule
                // if scfsi[scfsi_band] == 0, then each granule has its own scaling factors
                side_info->scfsi[ch][scfsi_band] = readBitsInc(buffer, &count, 1) != 0;

        for (int gr = 0; gr < 2; gr++)
            for (uint32_t ch = 0; ch < header->channels(); ch++) {
                // length of the scaling factors and main data in bits
                side_info->part2_3_length[gr][ch] = (int)readBitsInc(buffer, &count, 12);
                // number of values in each big_region
                side_info->big_value[gr][ch] = (int)readBitsInc(buffer, &count, 9);
                // quantizer step size
                side_info->global_gain[gr][ch] = (int)readBitsInc(buffer, &count, 8);
                // used to determine the values of slen1 and slen2
                side_info->scalefac_compress[gr][ch] = (int)readBitsInc(buffer, &count, 4);
                // number of bits given to a range of scale factors.
                side_info->slen1[gr][ch] = kSlenTable[side_info->scalefac_compress[gr][ch]][0];
                side_info->slen2[gr][ch] = kSlenTable[side_info->scalefac_compress[gr][ch]][1];
                // if set, a not normal window is used
                side_info->window_switching[gr][ch] = readBitsInc(buffer, &count, 1) == 1;

                if (side_info->window_switching[gr][ch]) {
                    // the window type for the granule, 2 is special
                    side_info->block_type[gr][ch] = (int)readBitsInc(buffer, &count, 2);
                    // number of scale factor bands before window switching
                    side_info->mixed_block_flag[gr][ch] = readBitsInc(buffer, &count, 1) == 1;
                    if (side_info->mixed_block_flag[gr][ch]) {
                        side_info->switch_point_l[gr][ch] = 8;
                        side_info->switch_point_s[gr][ch] = 3;
                    } else {
                        side_info->switch_point_l[gr][ch] = 0;
                        side_info->switch_point_s[gr][ch] = 0;
                    }

                    // these are set by default if window_switching
                    side_info->region0_count[gr][ch] = side_info->block_type[gr][ch] == 2 ? 8 : 7;
                    // no third region
                    side_info->region1_count[gr][ch] = 20 - side_info->region0_count[gr][ch];

                    for (int region = 0; region < 2; region++)
                        // huffman table number for a big region
                        side_info->table_select[gr][ch][region] = (int)readBitsInc(buffer, &count, 5);
                    for (int window = 0; window < 3; window++)
                        side_info->subblock_gain[gr][ch][window] = (int)readBitsInc(buffer, &count, 3);
                } else {
                    // set by default if !window_switching
                    side_info->block_type[gr][ch] = 0;
                    side_info->mixed_block_flag[gr][ch] = false;

                    for (int region = 0; region < 3; region++)
                        side_info->table_select[gr][ch][region] = (int)readBitsInc(buffer, &count, 5);

                    // number of scale factor bands in the first big value region
                    side_info->region0_count[gr][ch] = (int)readBitsInc(buffer, &count, 4);
                    // number of scale factor bands in the third big value region
                    side_info->region1_count[gr][ch] = (int)readBitsInc(buffer, &count, 3);
                }

                // if set, add values from a table to the scaling factors
                side_info->preflag[gr][ch] = (int)readBitsInc(buffer, &count, 1);
                // determines the step size
                side_info->scalefac_scale[gr][ch] = (int)readBitsInc(buffer, &count, 1);
                // table that determines which count1 table is used
                side_info->count1table_select[gr][ch] = (int)readBitsInc(buffer, &count, 1);
            }

    }

    void MP3FrameDecoder::unpackScalefacs(uint8_t* main_data, uint32_t granule, uint32_t channel, int &bit) {
        auto slen = kSlenTable[side_info->scalefac_compress[granule][channel]];
        if (side_info->block_type[granule][channel] == 2 && side_info->window_switching[granule][channel]) {
            if (side_info->mixed_block_flag[granule][channel]) {
                for (int i = 0; i < 8; i++) {
                    scalefac_l[granule][channel][i] = (int)readBitsInc(main_data, &bit, slen[0]);
                }

                for (int j = 3; j < 6; j++) {
                    for (int i = 0; i < 3; i++) {
                        scalefac_s[granule][channel][i][j] = (int)readBitsInc(main_data, &bit, slen[0]);
                    }
                }
            } else {
                for (int j = 0; j < 6; j++) {
                    for (int i = 0; i < 3; i++) {
                        scalefac_s[granule][channel][i][j] = (int)readBitsInc(main_data, &bit, slen[0]);
                    }
                }
            }

            for (int j = 6; j < 12; j++) {
                for (int i = 0; i < 3; i++) {
                    scalefac_s[granule][channel][i][j] = (int)readBitsInc(main_data, &bit, slen[1]);
                }
            }

            for (int i = 0; i < 3; i++) {
                scalefac_s[granule][channel][i][12] = 0;
            }
        } else {
            if (granule == 0) {
                for (int i = 0; i < 21; i++) {
                    if (i < 11) {
                        scalefac_l[granule][channel][i] = readBitsInc(main_data, &bit, slen[0]);
                    } else {
                        scalefac_l[granule][channel][i] = readBitsInc(main_data, &bit, slen[1]);
                    }
                }
            } else {
                auto scfsi = side_info->scfsi[channel];
                for (int i = 0; i < 21; i++) {
                    if (i < 6) {
                        if (scfsi[0]) {
                            scalefac_l[granule][channel][i] = scalefac_l[0][channel][i];
                        } else {
                            scalefac_l[granule][channel][i] = readBitsInc(main_data, &bit, slen[0]);
                        }
                    } else if (i < 11) {
                        if (scfsi[1]) {
                            scalefac_l[granule][channel][i] = scalefac_l[0][channel][i];
                        } else {
                            scalefac_l[granule][channel][i] = readBitsInc(main_data, &bit, slen[0]);
                        }
                    } else if (i < 16) {
                        if (scfsi[2]) {
                            scalefac_l[granule][channel][i] = scalefac_l[0][channel][i];
                        } else {
                            scalefac_l[granule][channel][i] = readBitsInc(main_data, &bit, slen[1]);
                        }
                    } else {
                        if (scfsi[3]) {
                            scalefac_l[granule][channel][i] = scalefac_l[0][channel][i];
                        } else {
                            scalefac_l[granule][channel][i] = readBitsInc(main_data, &bit, slen[1]);
                        }
                    }
                }
                scalefac_l[granule][channel][21] = 0;
            }
        }
    }

    void MP3FrameDecoder::unpackSamples(uint8_t* main_data, int gr, int ch, int bit, int max_bit) {
        int sample = 0;
        int table_num;
        HuffmanTree* table;

        for (int i = 0; i < 576; i++) {
            samples[gr][ch][i] = 0;
        }

        // get the big value region boundaries
        int region0;
        int region1;
        if (side_info->window_switching[gr][ch] && side_info->block_type[gr][ch] == 2) {
            region0 = 36;
            region1 = 576;
        } else {
            region0 = band_index.long_win[side_info->region0_count[gr][ch] + 1];
            region1 = band_index.long_win[side_info->region0_count[gr][ch] + 1 + side_info->region1_count[gr][ch] + 1];
        }

        // get the samples in the big value region
        // IMPORTANT: each entry in the Huffman table yields two samples
        for (; sample < (int)side_info->big_value[gr][ch] * 2; sample += 2) {
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

            // use the Huffman table and find a matching bit pattern
            int* values = table->getSampleValues(main_data, &bit);
            for (int i = 0; i < 2; i++) {

                // linbits extends the sample's size if needed
                int linbit = 0;
                if (kHuffmanTableMetadata[table_num][2] != 0 && values[i] == (int)max(kHuffmanTableMetadata[table_num][0], kHuffmanTableMetadata[table_num][1]) - 1)
                    linbit = (int)readBitsInc(main_data, &bit, kHuffmanTableMetadata[table_num][2]);

                // if the sample is negative or positive.
                int sign = 1;
                if (values[i] > 0)
                    sign = readBitsInc(main_data, &bit, 1) ? -1 : 1;

                samples[gr][ch][sample + i] = (float)(sign * (values[i] + linbit));
            }

        }

        // quadruples region
        for (; bit < max_bit && sample + 4 < 576; sample += 4) {
            int values[4];

            // flip bits
            if (side_info->count1table_select[gr][ch] == 1) {
                unsigned bit_sample = readBitsInc(main_data, &bit, 4);
                values[0] = (bit_sample & 0x08) > 0 ? 0 : 1;
                values[1] = (bit_sample & 0x04) > 0 ? 0 : 1;
                values[2] = (bit_sample & 0x02) > 0 ? 0 : 1;
                values[3] = (bit_sample & 0x01) > 0 ? 0 : 1;
            } else {
                unsigned bit_sample = readBits(main_data, bit, bit + 32);
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
                if (values[i] > 0 && readBitsInc(main_data, &bit, 1) == 1) {
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

    void MP3FrameDecoder::requantize(uint32_t gr, uint32_t ch) {
        using namespace util;
        float exp1, exp2;
        int window = 0;
        int sfb = 0;
        const float scalefac_mult = side_info->scalefac_scale[gr][ch] == 0 ? 0.5 : 1;

        for (uint32_t sample = 0, i = 0; sample < 576; sample++, i++) {
            if (side_info->block_type[gr][ch] == 2 || (side_info->mixed_block_flag[gr][ch] && sfb >= 8)) {
                if (i == band_width.short_win[sfb]) {
                    i = 0;
                    if (window == 2) {
                        window = 0;
                        sfb++;
                    } else
                        window++;
                }

                exp1 = side_info->global_gain[gr][ch] - 210.0 - 8.0 * side_info->subblock_gain[gr][ch][window];
                exp2 = scalefac_mult * scalefac_s[gr][ch][window][sfb];
            } else {
                if (sample == band_index.long_win[sfb + 1])
                    /* Don't increment sfb at the zeroth sample. */
                    sfb++;

                exp1 = side_info->global_gain[gr][ch] - 210.0;
                exp2 = scalefac_mult * (scalefac_l[gr][ch][sfb] + side_info->preflag[gr][ch] * kPretab[sfb]);
            }

            float sign = samples[gr][ch][sample] < 0 ? -1.0f : 1.0f;
            float a = math::power(math::abs(samples[gr][ch][sample]), 4.0 / 3.0);
            float b = math::power(2.0, exp1 / 4.0);
            float c = math::power(2.0, -exp2);

            samples[gr][ch][sample] = sign * a * b * c;
        }
    }

    void MP3FrameDecoder::midSideStereo(uint32_t gr) {
        for (int sample = 0; sample < 576; sample++) {
            float middle = samples[gr][0][sample];
            float side = samples[gr][1][sample];
            samples[gr][0][sample] = (middle + side) / util::math::M_SQRT2;
            samples[gr][1][sample] = (middle - side) / util::math::M_SQRT2;
        }
    }

    void MP3FrameDecoder::reorder(uint32_t gr, uint32_t ch) {
        int total = 0;
        int start = 0;
        int block = 0;
        float samples[576] = {0};

        for (int sb = 0; sb < 12; sb++) {
            const int sb_width = band_width.short_win[sb];

            for (int ss = 0; ss < sb_width; ss++) {
                samples[start + block + 0] = this->samples[gr][ch][total + ss + sb_width * 0];
                samples[start + block + 6] = this->samples[gr][ch][total + ss + sb_width * 1];
                samples[start + block + 12] = this->samples[gr][ch][total + ss + sb_width * 2];

                if (block != 0 && block % 5 == 0) { /* 6 * 3 = 18 */
                    start += 18;
                    block = 0;
                } else
                    block++;
            }

            total += sb_width * 3;
        }

        for (int i = 0; i < 576; i++)
            this->samples[gr][ch][i] = samples[i];
    }

    void MP3FrameDecoder::aliasReduction(uint32_t granule, uint32_t channel) {
        int sb_max = side_info->mixed_block_flag[granule][channel] ? 2 : 32;
        for (int sb = 1; sb < sb_max; sb++)
            for (int sample = 0; sample < 8; sample++) {
                int offset1 = 18 * sb - sample - 1;
                int offset2 = 18 * sb + sample;
                float s1 = samples[granule][channel][offset1];
                float s2 = samples[granule][channel][offset2];
                samples[granule][channel][offset1] = s1 * kCS[sample] - s2 * kCA[sample];
                samples[granule][channel][offset2] = s2 * kCS[sample] + s1 * kCA[sample];
            }
    }

    void MP3FrameDecoder::frequencyInversion(uint32_t granule, uint32_t channel) {
        for (int sb = 1; sb < 18; sb += 2)
            for (int i = 1; i < 32; i += 2)
                samples[granule][channel][i * 18 + sb] *= -1;
    }

    void MP3FrameDecoder::IMDCT(uint32_t gr, uint32_t ch) {
        static bool init = true;
        static float sine_block[4][36];
        float sample_block[36];

        if (init) {
            int i;
            for (i = 0; i < 36; i++)
                sine_block[0][i] = util::math::sin(util::math::M_PI / 36.0 * (i + 0.5));
            for (i = 0; i < 18; i++)
                sine_block[1][i] = util::math::sin(util::math::M_PI / 36.0 * (i + 0.5));
            for (; i < 24; i++)
                sine_block[1][i] = 1.0;
            for (; i < 30; i++)
                sine_block[1][i] = util::math::sin(util::math::M_PI / 12.0 * (i - 18.0 + 0.5));
            for (; i < 36; i++)
                sine_block[1][i] = 0.0;
            for (i = 0; i < 12; i++)
                sine_block[2][i] = util::math::sin(util::math::M_PI / 12.0 * (i + 0.5));
            for (i = 0; i < 6; i++)
                sine_block[3][i] = 0.0;
            for (; i < 12; i++)
                sine_block[3][i] = util::math::sin(util::math::M_PI / 12.0 * (i - 6.0 + 0.5));
            for (; i < 18; i++)
                sine_block[3][i] = 1.0;
            for (; i < 36; i++)
                sine_block[3][i] = util::math::sin(util::math::M_PI / 36.0 * (i + 0.5));
            init = false;
        }

        const int n = side_info->block_type[gr][ch] == 2 ? 12 : 36;
        const int half_n = n / 2;
        int sample = 0;

        for (int block = 0; block < 32; block++) {
            for (int win = 0; win < (side_info->block_type[gr][ch] == 2 ? 3 : 1); win++) {
                for (int i = 0; i < n; i++) {
                    float xi = 0.0;
                    for (int k = 0; k < half_n; k++) {
                        float s = samples[gr][ch][18 * block + half_n * win + k];
                        xi += s * util::math::cos(util::math::M_PI / (2 * n) * (2 * i + 1 + half_n) * (2 * k + 1));
                    }

                    /* Windowing samples. */
                    sample_block[win * n + i] = xi * sine_block[side_info->block_type[gr][ch]][i];
                }
            }

            if (side_info->block_type[gr][ch] == 2) {
                float temp_block[36];
                memcpy(temp_block, sample_block, 36 * 4);

                int i = 0;
                for (; i < 6; i++)
                    sample_block[i] = 0;
                for (; i < 12; i++)
                    sample_block[i] = temp_block[0 + i - 6];
                for (; i < 18; i++)
                    sample_block[i] = temp_block[0 + i - 6] + temp_block[12 + i - 12];
                for (; i < 24; i++)
                    sample_block[i] = temp_block[12 + i - 12] + temp_block[24 + i - 18];
                for (; i < 30; i++)
                    sample_block[i] = temp_block[24 + i - 18];
                for (; i < 36; i++)
                    sample_block[i] = 0;
            }

            /* Overlap. */
            for (int i = 0; i < 18; i++) {
                samples[gr][ch][sample + i] = sample_block[i] + prev_samples[ch][block][i];
                prev_samples[ch][block][i] = sample_block[18 + i];
            }
            sample += 18;
        }
    }

    void MP3FrameDecoder::synthFilterbank(uint32_t gr, uint32_t ch) {
        static float n[64][32];
        static bool init = true;

        if (init) {
            init = false;
            for (int i = 0; i < 64; i++)
                for (int j = 0; j < 32; j++)
                    n[i][j] = util::math::cos((16.0 + i) * (2.0 * j + 1.0) * (util::math::M_PI / 64.0));
        }

        float s[32], u[512], w[512];
        float pcm[576];

        for (int sb = 0; sb < 18; sb++) {
            for (int i = 0; i < 32; i++)
                s[i] = samples[gr][ch][i * 18 + sb];

            for (int i = 1023; i > 63; i--)
                fifo[ch][i] = fifo[ch][i - 64];

            for (int i = 0; i < 64; i++) {
                fifo[ch][i] = 0.0;
                for (int j = 0; j < 32; j++)
                    fifo[ch][i] += s[j] * n[i][j];
            }

            for (int i = 0; i < 8; i++)
                for (int j = 0; j < 32; j++) {
                    u[i * 64 + j] = fifo[ch][i * 128 + j];
                    u[i * 64 + j + 32] = fifo[ch][i * 128 + j + 96];
                }

            for (int i = 0; i < 512; i++)
                w[i] = u[i] * kSynthWindow[i];

            for (int i = 0; i < 32; i++) {
                float sum = 0;
                for (int j = 0; j < 16; j++)
                    sum += w[j * 32 + i];
                pcm[32 * sb + i] = sum;
            }
        }

        memcpy(samples[gr][ch], pcm, 576 * 4);
    }

    int16_t scalePCM(float sample) {
        if (sample >=  32766.5) return (int16_t) 32767;
        if (sample <= -32767.5) return (int16_t)-32768;
        int16_t s = (int16_t)(sample + .5f);
        s -= (s < 0);
        return s;
    }

    void MP3FrameDecoder::interleave() {
        int i = 0;
        for (int gr = 0; gr < 2; gr++)
            for (int sample = 0; sample < 576; sample++)
                for (uint32_t ch = 0; ch < header->channels(); ch++)
                    pcm[i++] = scalePCM(samples[gr][ch][sample]);

        for (int j = 0; j < i; j++) std::cout << pcm[j] << ((j+1)%20 ? ' ' : '\n');
        if(i%20) std::cout << '\n';
    }

    void MP3SideInfo::printSideInfo() {
        printf("************ SIDE INFO ************\n");
        printf("\tmain_data_begin: %d\n", main_data_begin);
        printf("\tscsfi: ");
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                printf("%d ", scfsi[i][j]);
            }
        }
        printf("\n");

        for (int i = 0; i < 2; i++) {
            printf("************ GRANULE %d ***********\n", i + 1);
            for (int j = 0; j < 2; j++) {
                printf("************ CHANNEL %d ***********\n", j + 1);
                printf("\tpart2_3_length: %d\n", part2_3_length[i][j]);
                printf("\tpart2_length: %d\n", part2_length[i][j]);
                printf("\tbig_value: %d\n", big_value[i][j]);
                printf("\tglobal_gain: %d\n", global_gain[i][j]);
                printf("\tscalefac_compress: %d\n", scalefac_compress[i][j]);
                printf("\tslen1: %d\n", slen1[i][j]);
                printf("\tslen2: %d\n", slen2[i][j]);
                printf("\twindow_switching: %d\n", window_switching[i][j]);
                printf("\tblock_type: %d\n", block_type[i][j]);
                printf("\tmixed_block_flag: %d\n", mixed_block_flag[i][j]);
                printf("\tswitch_point_l: %d\n", switch_point_l[i][j]);
                printf("\tswitch_point_s: %d\n", switch_point_s[i][j]);
                printf("\ttable_select: ");
                for (int k = 0; k < 3; k++) {
                    printf("%d ", table_select[i][j][k]);
                }
                printf("\n\tsubblock_gain: ");
                for (int k = 0; k < 3; k++) {
                    printf("%d ", subblock_gain[i][j][k]);
                }
                printf("\n\tregion0_count: %d\n", region0_count[i][j]);
                printf("\tregion1_count: %d\n", region1_count[i][j]);
                printf("\tpreflag: %d\n", preflag[i][j]);
                printf("\tscalefac_scale: %d\n", scalefac_scale[i][j]);
                printf("\tcount1table_select: %d\n", count1table_select[i][j]);
            }
        }
    }

    /*MP3::MP3(uint8_t* data) : data(data) {
            id3_tag = new ID3{};
            for (int i = 0; i < 10; i++) {
                printf("%02x\t", data[i]);
                ((uint8_t*)id3_tag)[i] = data[i];
            }
            printf("\nIDT: %c, %c, %c\n", id3_tag->i, id3_tag->d, id3_tag->t);
            if (id3_tag->isID3()) printf("This is an ID3 with size: %d\n", id3_tag->size);
            current_location += 70;
            decoder = new MP3FrameDecoder();
    }

    bool MP3::readNextFrame() {
        frame_number++;
        uint32_t new_offset = decoder->readFrame(data+current_location);
        if (new_offset == 0xFFFFFFFF) {
            printf("Next frame_sync: %d\n", decoder->header->frame_sync);
            return false;
        }
        current_location += new_offset;
        return true;
    }*/

}

}

}
