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
#include "OutputStream.h"

class VideoStream : public OutputStream
{
public:
    VideoStream(AVObject* parent);
    static AVFrame* allocateFrame(AVCodecContext* codec_ctx);
    static AVFrame* allocateFrame(AVPixelFormat pix_fmt, int width, int height);
    int writeFrame(AVFrame* frame_in) override;
    void close() override;

    AVObject* parent;
};

