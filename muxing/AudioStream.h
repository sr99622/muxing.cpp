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
#include "StreamParameters.h"
#include "CircularQueue.h"

class AudioStream 
{
public:
    AudioStream(void* parent, const StreamParameters& params, CircularQueue<AVPacket*>* pkt_q);
    ~AudioStream();
    int writeFrame(AVFrame* frame_in);
    void close();

    void* parent;

    AVStream* stream;
    AVCodecContext* enc;
    AVPacket* pkt;
    AVExceptionHandler av;

    CircularQueue<AVPacket*>* pkt_q;
};
