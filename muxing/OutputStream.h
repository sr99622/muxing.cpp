#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "../Utilities/avexception.h"

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

class OutputStream {
public:

    virtual void close();
    virtual int writeFrame(AVFrame* frame_out);

    AVStream* stream;
    AVCodecContext* enc;
    AVCodec* codec;
    AVFormatContext* fmt_ctx;
    AVFrame* frame;
    AVPacket* pkt;
    AVExceptionHandler av;
};

