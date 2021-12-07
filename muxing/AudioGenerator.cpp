#include "AudioGenerator.h"

AudioGenerator::AudioGenerator(AVCodecContext* enc)
{
    time_base = enc->time_base;
    channels = enc->channels;
    t = 0;
    next_pts = 0;
    tincr = 2 * M_PI * 110.0 / enc->sample_rate;
    tincr2 = 2 * M_PI * 110.0 / enc->sample_rate / enc->sample_rate;

    int nb_samples;
    if (enc->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = enc->frame_size;

    try {
        av.ck(frame = AudioStream::allocateFrame(AV_SAMPLE_FMT_S16, enc->channel_layout, enc->sample_rate, nb_samples));
    }
    catch (const std::exception& e) {
        std::cerr << "AudioStream::allocateFrame exception: " << e.what() << std::endl;
    }
}

AudioGenerator::~AudioGenerator()
{
    av_frame_free(&frame);
}

AVFrame* AudioGenerator::getFrame()
{
    int j, i, v;
    int16_t* q = (int16_t*)frame->data[0];

    // check if we want to generate more frames 
    if (av_compare_ts(next_pts, time_base, STREAM_DURATION, av_make_q(1, 1)) > 0)
        return NULL;

    for (j = 0; j < frame->nb_samples; j++) {
        v = (int)(sin(t) * 10000);
        for (i = 0; i < channels; i++)
            *q++ = v;
        t += tincr;
        tincr += tincr2;
    }

    frame->pts = next_pts;
    next_pts += frame->nb_samples;

    return frame;
}
