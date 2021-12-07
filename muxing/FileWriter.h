#pragma once

extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include "../Utilities/avexception.h"

#include "AVObject.h"
#include "AudioStream.h"
#include "VideoStream.h"

struct StreamParameters
{
    // VIDEO
    AVPixelFormat pix_fmt;
    int width;
    int height;
    int video_bit_rate;
    int frame_rate;
    int gop_size;

    // AUDIO
    AVSampleFormat sample_fmt;
    uint64_t channel_layout;
    int audio_bit_rate;
    int sample_rate;

    AVDictionary* opts = NULL;
};

class FileWriter : public AVObject
{
public:
    FileWriter(const char* filename, StreamParameters* params);
    void close();

    AVFormatContext* fmt_ctx;
    VideoStream* videoStream = NULL;
    AudioStream* audioStream = NULL;
    AVExceptionHandler av;
    StreamParameters* params;
};

