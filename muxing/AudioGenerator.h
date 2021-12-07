#pragma once

#include "FileWriter.h"

class AudioGenerator
{
public:
    AudioGenerator(AVCodecContext* enc);
    ~AudioGenerator();
    AVFrame* getFrame();

    float t, tincr, tincr2;
    int64_t next_pts;
    AVFrame* frame;
    AVRational time_base;
    int channels;
    AVExceptionHandler av;

};

