#pragma once

#include <iostream>

typedef struct {
    int16_t L;
    int16_t R;
} sample_16b_2ch_t;

typedef struct {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;    // Bytes per second
    uint16_t blockAlign;  // Bytes per sample, bitePerSample * numChannels / 8
    uint16_t bitsPerSample;
    uint32_t dataSize;
    uint32_t numSamples;
    sample_16b_2ch_t* sample;
} wav_t;

int loadWav(const char path[], wav_t& WAV);
void freeWav(wav_t& WAV);
void printMeta(wav_t& WAV);
