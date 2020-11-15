#include "mp3.h"

namespace io {

namespace audio {

namespace mp3 {

    MP3Frame::MP3Frame(uint8_t* data) : ref_count(0) {
        header = new MP3FrameHeader{};
        MP3SideInfoPrelim* side_info_prelim = new MP3SideInfoPrelim{};

        //Skip the ID3v2 for now
        data+=70;

        //Load the header data
        for(int i = 0; i < 4; i++) ((uint8_t*)header)[i] = data[3-i];
        data+=4;

        //Skip the CRC (if protection_bit is 0)
        if (!header->protection_bit)
            data+=2;

        //Load the side_info_prelim
        for(int i = 0; i < 32; i++) ((uint8_t*)side_info_prelim)[i] = data[31 - i];
        for(int i = 0; i < 32; i++) printf("%02x\t",data[i]);
        printf("\n");
        //memcpy((void*)side_info_prelim, data, 32);

        side_info->main_data_begin = side_info_prelim->main_data_begin;
        side_info->scsfi = side_info_prelim->scsfi;
        side_info->private_bits = side_info_prelim->private_bits;

        side_info->count1table_select[0][0] = side_info_prelim->count1table_select_1_c1;
        side_info->count1table_select[0][1] = side_info_prelim->count1table_select_1_c2;
        side_info->count1table_select[1][0] = side_info_prelim->count1table_select_2_c1;
        side_info->count1table_select[1][1] = side_info_prelim->count1table_select_2_c2;
        side_info->scalefac_scale[0][0] = side_info_prelim->scalefac_scale_1_c1;
        side_info->scalefac_scale[0][1] = side_info_prelim->scalefac_scale_1_c2;
        side_info->scalefac_scale[1][0] = side_info_prelim->scalefac_scale_2_c1;
        side_info->scalefac_scale[1][1] = side_info_prelim->scalefac_scale_2_c2;
        side_info->preflag[0][0] = side_info_prelim->preflag_1_c1;
        side_info->preflag[0][1] = side_info_prelim->preflag_1_c2;
        side_info->preflag[1][0] = side_info_prelim->preflag_2_c1;
        side_info->preflag[1][1] = side_info_prelim->preflag_2_c2;
        side_info->region1_count[0][0] = side_info_prelim->region1_count_1_c1;
        side_info->region1_count[0][1] = side_info_prelim->region1_count_1_c2;
        side_info->region1_count[1][0] = side_info_prelim->region1_count_2_c1;
        side_info->region1_count[1][1] = side_info_prelim->region1_count_2_c2;
        side_info->region0_count[0][0] = side_info_prelim->region0_count_1_c1;
        side_info->region0_count[0][1] = side_info_prelim->region0_count_1_c2;
        side_info->region0_count[1][0] = side_info_prelim->region0_count_2_c1;
        side_info->region0_count[1][1] = side_info_prelim->region0_count_2_c2;
        side_info->table_select[0][0] = side_info_prelim->table_select_1_c1;
        side_info->table_select[0][1] = side_info_prelim->table_select_1_c2;
        side_info->table_select[1][0] = side_info_prelim->table_select_2_c1;
        side_info->table_select[1][1] = side_info_prelim->table_select_2_c2;
        side_info->windows_switching_flag[0][0] = side_info_prelim->windows_switching_flag_1_c1;
        side_info->windows_switching_flag[0][1] = side_info_prelim->windows_switching_flag_1_c2;
        side_info->windows_switching_flag[1][0] = side_info_prelim->windows_switching_flag_2_c1;
        side_info->windows_switching_flag[1][1] = side_info_prelim->windows_switching_flag_2_c2;
        side_info->scalefac_compress[0][0] = side_info_prelim->scalefac_compress_1_c1;
        side_info->scalefac_compress[0][1] = side_info_prelim->scalefac_compress_1_c2;
        side_info->scalefac_compress[1][0] = side_info_prelim->scalefac_compress_2_c1;
        side_info->scalefac_compress[1][1] = side_info_prelim->scalefac_compress_2_c2;
        side_info->global_gain[0][0] = side_info_prelim->global_gain_1_c1;
        side_info->global_gain[0][1] = side_info_prelim->global_gain_1_c2;
        side_info->global_gain[1][0] = side_info_prelim->global_gain_2_c1;
        side_info->global_gain[1][1] = side_info_prelim->global_gain_2_c2;
        side_info->big_values[0][0] = side_info_prelim->big_values_1_c1;
        side_info->big_values[0][1] = side_info_prelim->big_values_1_c2;
        side_info->big_values[1][0] = side_info_prelim->big_values_2_c1;
        side_info->big_values[1][1] = side_info_prelim->big_values_2_c2;
        side_info->part2_3_length[0][0] = side_info_prelim->part2_3_length_1_c1;
        side_info->part2_3_length[0][1] = side_info_prelim->part2_3_length_1_c2;
        side_info->part2_3_length[1][0] = side_info_prelim->part2_3_length_2_c1;
        side_info->part2_3_length[1][1] = side_info_prelim->part2_3_length_2_c2;
    }

    MP3Frame::~MP3Frame() {

    }

}

}

}
