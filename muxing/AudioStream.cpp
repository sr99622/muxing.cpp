#include "AudioStream.h"
#include "FileWriter.h"

AudioStream::AudioStream(AVObject* parent)
{
    this->parent = parent;
    FileWriter* writer = ((FileWriter*)parent);
    fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = fmt_ctx->oformat->audio_codec;

    try {
        av.ck(codec = avcodec_find_encoder(codec_id), codec_id, CmdTag::AFE);
        av.ck(pkt = av_packet_alloc(), CmdTag::APA);
        av.ck(stream = avformat_new_stream(fmt_ctx, NULL), CmdTag::ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        av.ck(enc = avcodec_alloc_context3(codec), CmdTag::AAC3);

        enc->sample_fmt = codec->sample_fmts ? codec->sample_fmts[0] : writer->params->sample_fmt;
        enc->bit_rate = writer->params->audio_bit_rate;
        enc->sample_rate = writer->params->sample_rate;

        /*
        if (codec->supported_samplerates) {
            enc->sample_rate = codec->supported_samplerates[0];
            for (int i = 0; codec->supported_samplerates[i]; i++) {
                if (codec->supported_samplerates[i] == 44100)
                    enc->sample_rate = 44100;
            }
        }
        */
        //enc->channels = av_get_channel_layout_nb_channels(enc->channel_layout);

        enc->channel_layout = writer->params->channel_layout;

        /*
        if (codec->channel_layouts) {
            enc->channel_layout = codec->channel_layouts[0];
            for (int i = 0; codec->channel_layouts[i]; i++) {
                if (codec->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    enc->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        */

        enc->channels = av_get_channel_layout_nb_channels(enc->channel_layout);
        stream->time_base = av_make_q(1, enc->sample_rate);

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        av.ck(avcodec_open2(enc, codec, &writer->params->opts), CmdTag::AO2);

        int nb_samples;
        if (enc->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
            nb_samples = 10000;
        else
            nb_samples = enc->frame_size;

        av.ck(frame = allocateFrame(enc->sample_fmt, enc->channel_layout, enc->sample_rate, nb_samples));
        av.ck(avcodec_parameters_from_context(stream->codecpar, enc), CmdTag::APFC);
        av.ck(swr_ctx = swr_alloc(), CmdTag::SA);
        av_opt_set_int(swr_ctx, "in_channel_count", enc->channels, 0);
        av_opt_set_int(swr_ctx, "in_sample_rate", enc->sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int(swr_ctx, "out_channel_count", enc->channels, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate", enc->sample_rate, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", enc->sample_fmt, 0);
        av.ck(swr_init(swr_ctx), CmdTag::SI);
    }
    catch (const AVException& e) {
        std::cerr << "AudioStream constructor exception: " << e.what() << std::endl;
        close();
    }
}

void AudioStream::close()
{
    swr_free(&swr_ctx);
    OutputStream::close();
}

AVFrame* AudioStream::allocateFrame(AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples)
{
    AVFrame* frame = NULL;
    AVExceptionHandler av;

    try {
        av.ck(frame = av_frame_alloc(), CmdTag::AFA);
        frame->format = sample_fmt;
        frame->channel_layout = channel_layout;
        frame->sample_rate = sample_rate;
        frame->nb_samples = nb_samples;
        if (nb_samples) {
            av.ck(av_frame_get_buffer(frame, 0), CmdTag::AFGB);
        }
        av.ck(av_frame_make_writable(frame), CmdTag::AFMW);
    }
    catch (const std::exception& e) {
        std::cerr << "AudioStream::allocateFrame exception: " << e.what() << std::endl;
        if (frame) {
            av_frame_free(&frame);
            frame = NULL;
        }
    }

    return frame;
}

int AudioStream::writeFrame(AVFrame* frame_in)
{
    int ret;
    int dst_nb_samples;

    if (frame_in) {
        try {
            dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, enc->sample_rate) + frame_in->nb_samples,
                enc->sample_rate, enc->sample_rate, AV_ROUND_UP);

            av_assert0(dst_nb_samples == frame->nb_samples);

            av.ck(ret = swr_convert(swr_ctx, frame->data, dst_nb_samples,
                (const uint8_t**)frame_in->data, frame_in->nb_samples), CmdTag::SC);

            frame_in = frame;
            frame_in->pts = av_rescale_q(samples_count, av_make_q(1, enc->sample_rate), enc->time_base);
            samples_count += dst_nb_samples;
        }
        catch (const AVException& e) {
            std::cerr << "AudioStream::writeFrame exception: " << e.what() << std::endl;
        }
    }

    return OutputStream::writeFrame(frame_in);
}

