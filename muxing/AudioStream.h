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

class AudioStream : public OutputStream
{
public:
    AudioStream(AVObject* parent);
    static AVFrame* allocateFrame(AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples);
    int writeFrame(AVFrame* frame_in) override;
    void close() override;

    AVObject* parent;
    struct SwrContext* swr_ctx;
    int samples_count;
};
