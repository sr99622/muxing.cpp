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

#include "AudioStream.h"
#include "VideoStream.h"
#include "StreamParameters.h"

class FileWriter
{
public:
    FileWriter(const StreamParameters& params);
    ~FileWriter();
    void openFile(const char* filename);
    void writeFrame(AVPacket* pkt);
    void closeFile();

    AVFormatContext* fmt_ctx;
    //VideoStream* videoStream = NULL;
    //AudioStream* audioStream = NULL;
    AVDictionary* opts;
    AVExceptionHandler av;

};

