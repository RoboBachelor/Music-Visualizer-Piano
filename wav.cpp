
//  wav.cpp
//
//  Created by Jingyi Wang on 2021/11/6.
//

#include <iostream>
#include <fstream>
#include <iomanip>

#include "wav.h"

void printMeta(wav_t& WAV) {
    printf("Num channels: %d\nSampling rate: %d\nByte rate: %d\nBlock Align: %d\nBits per sample: %d\nData size: %d\n",
        WAV.numChannels, WAV.sampleRate, WAV.byteRate, WAV.blockAlign, WAV.bitsPerSample, WAV.dataSize);
}

int loadWav(const char path[], wav_t& WAV) {

    std::fstream fs;
    fs.open(path, std::ios::binary | std::ios::in);
    
    char descriptor[5] = { 0 };
    uint64_t fileOffset = 8;

    fs.seekg(fileOffset);
    fs.read(descriptor, 4);
    if (strcmp(descriptor, "WAVE") != 0) {
        return -1;
    }
    fileOffset += 4;

    while (1) {
        uint32_t chunk_size;

        fs.seekg(fileOffset);
        fs.read(descriptor, 4);
        fs.seekg(fileOffset += 4);
        fs.read((char*) &chunk_size, 4);
        fileOffset += 4;

        if (strcmp(descriptor, "fmt ") == 0) {
            fs.seekg(fileOffset);
            fs.read((char*)&WAV, 16);
        }

        if (strcmp(descriptor, "data") == 0) {
            WAV.dataSize = chunk_size;
            WAV.numSamples = WAV.dataSize >> 2;
            WAV.sample = new sample_16b_2ch_t[WAV.numSamples];

            fs.seekg(fileOffset);
            fs.read((char*)WAV.sample, WAV.dataSize);
            break;
        }

        fileOffset += chunk_size;
    }
    fs.close();
    return 0;
}

void freeWav(wav_t& WAV) {
    delete[] WAV.sample;
}
