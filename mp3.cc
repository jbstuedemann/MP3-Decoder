#include "mp3.h"

namespace io {

namespace audio {

namespace mp3 {
    // data points to the start of the frame header
    MP3Frame::MP3Frame(uint8_t* data) {
        header = new MP3FrameHeader{};
        auto side_info_prelim = new MP3SideInfoPrelim{};

        // load the header data
        for(int i = 0; i < 4; i++) ((uint8_t*)header)[i] = data[3-i];
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
        side_info_prelim->printSideInfo();
        side_info_prelim->printGranule_1();
        side_info_prelim->printGranule_2();

        side_info = new MP3SideInfo(side_info_prelim);
        free(side_info_prelim);
    }

    MP3Frame::~MP3Frame() {
        free(header);
        free(side_info);
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

}

}

}
