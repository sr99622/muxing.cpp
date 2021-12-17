#include "VideoStream.h"
#include "FileWriter.h"

VideoStream::VideoStream(AVObject* parent)
{
    this->parent = parent;
    FileWriter* writer = ((FileWriter*)parent);
    fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = fmt_ctx->oformat->video_codec;

    try {
        av.ck(codec = avcodec_find_encoder(codec_id), std::string("avcodec_find_decoder could not find ") + avcodec_get_name(codec_id));
        av.ck(pkt = av_packet_alloc(), CmdTag::APA);
        av.ck(stream = avformat_new_stream(fmt_ctx, NULL), CmdTag::ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        av.ck(enc = avcodec_alloc_context3(codec), CmdTag::AAC3);

        enc->codec_id = codec_id;
        enc->bit_rate = writer->params->video_bit_rate;
        enc->width = writer->params->width;
        enc->height = writer->params->height;
        stream->time_base = av_make_q(1, writer->params->frame_rate);
        enc->time_base = stream->time_base;

        enc->gop_size = writer->params->gop_size;
        enc->pix_fmt = writer->params->pix_fmt;

        if (enc->codec_id == AV_CODEC_ID_MPEG2VIDEO)
            enc->max_b_frames = 2;

        if (enc->codec_id == AV_CODEC_ID_MPEG1VIDEO)
            enc->mb_decision = 2;

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        av.ck(avcodec_open2(enc, codec, &writer->params->opts), CmdTag::AO2);
        av.ck(frame = allocateFrame(enc));
        av.ck(avcodec_parameters_from_context(stream->codecpar, enc), CmdTag::APFC);
    }
    catch (const AVException& e) {
        std::cerr << "VideoStream constructor exception: " << e.what() << std::endl;
        close();
    }
}

void VideoStream::close()
{
    OutputStream::close();
}

int VideoStream::writeFrame(AVFrame* frame)
{
    return OutputStream::writeFrame(frame);
}

AVFrame* VideoStream::allocateFrame(AVCodecContext* codec_ctx)
{
    AVFrame* frame = NULL;
    AVExceptionHandler av;

    try {
        av.ck(frame = av_frame_alloc(), CmdTag::AFA);
        frame->format = codec_ctx->pix_fmt;
        frame->width = codec_ctx->width;
        frame->height = codec_ctx->height;
        av.ck(av_frame_get_buffer(frame, 0), CmdTag::AFGB);
    }
    catch (const AVException& e) {
        std::cerr << "VideoStream::allocateFrame exception: " << e.what() << std::endl;
        if (frame) {
            av_frame_free(&frame);
            frame = NULL;
        }
    }

    return frame;
}

AVFrame* VideoStream::allocateFrame(AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame* frame = NULL;
    AVExceptionHandler av;

    try {
        av.ck(frame = av_frame_alloc(), CmdTag::AFA);
        frame->format = pix_fmt;
        frame->width = width;
        frame->height = height;
        av.ck(av_frame_get_buffer(frame, 0), CmdTag::AFGB);
    }
    catch (const AVException& e) {
        std::cerr << "VideoStream::allocateFrame exception: " << e.what() << std::endl;
        if (frame) {
            av_frame_free(&frame);
            frame = NULL;
        }
    }

    return frame;
}

