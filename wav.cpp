
//  main.cpp
//  PCM3.0    解码效率低 卡顿十分严重
//
//  Created by boone on 2018/8/9.
//  Copyright © 2018年 boone. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <iomanip>

struct data_16bits {
    int16_t L, R;
};

struct wav_struct
{
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;    // Bytes per second
    uint16_t blockAlign;  // Bytes per sample, bitePerSample * numChannels / 8
    uint16_t bitsPerSample;
    uint32_t dataSize;
    data_16bits* samples;
};

int main(int argc, char** argv)
{
    std::fstream fs;
    wav_struct WAV;
    fs.open("D:\\CloudMusic\\Cheetah Mobile Games - The Piano.wav", std::ios::binary | std::ios::in);
    
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
            WAV.samples = new data_16bits[WAV.dataSize >> 2];

            fs.seekg(fileOffset);
            fs.read((char*)WAV.samples, WAV.dataSize);
            break;
        }

        fileOffset += chunk_size;
    }
    fs.close();

    printf("Num channels: %d Sampling rate: %d Byte rate: %d Block Align: %d Bits per sample: %d Data size: %d\n", WAV.numChannels, WAV.sampleRate, WAV.byteRate, WAV.blockAlign, WAV.bitsPerSample, WAV.dataSize);


    for (uint32_t i = 100000 / 4; i < 100100 / 4; i++)
    {
        float floatData = WAV.samples[i].L / 32768.f;
        printf("%f ", floatData);

    }

    delete[] WAV.samples;
    system("pause");
}