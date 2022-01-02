#include "AudioStream.h"
#include "FileWriter.h"

AudioStream::AudioStream(void* parent, const StreamParameters& params, CircularQueue<AVPacket*>* pkt_q)
{
    this->parent = parent;
    this->pkt_q = pkt_q;
    FileWriter* writer = ((FileWriter*)parent);
    AVFormatContext* fmt_ctx = writer->fmt_ctx;
    AVCodecID codec_id = fmt_ctx->oformat->audio_codec;

    try {
        AVCodec* codec;
        av.ck(codec = avcodec_find_encoder(codec_id), std::string("avcodec_find_decoder could not find ") + avcodec_get_name(codec_id));
        av.ck(pkt = av_packet_alloc(), CmdTag::APA);
        av.ck(stream = avformat_new_stream(fmt_ctx, NULL), CmdTag::ANS);
        stream->id = fmt_ctx->nb_streams - 1;
        av.ck(enc = avcodec_alloc_context3(codec), CmdTag::AAC3);

        enc->sample_fmt = params.sample_fmt;
        enc->bit_rate = params.audio_bit_rate;
        enc->sample_rate = params.sample_rate;
        enc->channel_layout = params.channel_layout;
        enc->channels = av_get_channel_layout_nb_channels(enc->channel_layout);
        stream->time_base = av_make_q(1, enc->sample_rate);

        if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            enc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        AVDictionary* opts = params.opts;
        av.ck(avcodec_open2(enc, codec, &opts), CmdTag::AO2);
        av.ck(avcodec_parameters_from_context(stream->codecpar, enc), CmdTag::APFC);
    }
    catch (const AVException& e) {
        std::cerr << "AudioStream constructor exception: " << e.what() << std::endl;
        close();
    }
}

AudioStream::~AudioStream()
{
    close();
}

void AudioStream::close()
{
    avcodec_free_context(&enc);
    av_packet_free(&pkt);
}

int AudioStream::writeFrame(AVFrame* frame)
{
    int ret = 0;

    try {
        av.ck(avcodec_send_frame(enc, frame), CmdTag::ASF);

        while (ret >= 0) {
            ret = avcodec_receive_packet(enc, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                av.ck(ret, CmdTag::ARP);
            }

            av_packet_rescale_ts(pkt, enc->time_base, stream->time_base);
            pkt->stream_index = stream->index;

            AVPacket* tmp = av_packet_alloc();
            tmp = av_packet_clone(pkt);
            pkt_q->push(tmp);
        }

    }
    catch (const AVException& e) {
        std::cerr << "AudioStream::writeFrame exception: " << e.what() << std::endl;
    }

    return ret;
}

