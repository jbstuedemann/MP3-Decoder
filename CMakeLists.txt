cmake_minimum_required(VERSION 3.17)
project(MP3_Decoder)

set(CMAKE_CXX_STANDARD 14)

add_executable(MP3_Decoder main.cpp mp3.h mp3.cc huffman.h huffman.cc tables.h audio_util.h audio_util.cc math.h)