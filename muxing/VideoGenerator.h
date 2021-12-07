#pragma once

#include "FileWriter.h"

class VideoGenerator
{
public:
    VideoGenerator(AVCodecContext* enc);
    ~VideoGenerator();
    AVFrame* getFrame();
    void fillFrame(AVFrame* pict, int frame_index);

    int width;
    int height;
    int64_t next_pts;
    SwsContext* sws_ctx;
    AVPixelFormat pix_fmt;
    AVRational time_base;
    AVFrame* frame;
    AVFrame* yuv_frame;
    AVExceptionHandler av;
};

