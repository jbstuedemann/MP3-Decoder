#include <bits/stdc++.h>
#include "mp3.h"

using namespace std;

int main(){
    ifstream ifs;
    ifs.open ("../test.mp3", std::ifstream::in);

    uint8_t data [1000];
    ifs.read((char*)data, 1000);

    io::audio::mp3::MP3Frame* mp3File = new io::audio::mp3::MP3Frame(data);
    mp3File->header->printHeader();
    mp3File->side_info->printSideInfo();
    mp3File->side_info->printGranule_1();
    mp3File->side_info->printGranule_2();

    return 0;
}