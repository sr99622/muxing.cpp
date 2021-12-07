#include "OutputStream.h"

int OutputStream::writeFrame(AVFrame* frame_out)
{
    int ret = 0;
    try {
        av.ck(avcodec_send_frame(enc, frame_out), CmdTag::ASF);

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
            av.ck(av_interleaved_write_frame(fmt_ctx, pkt), CmdTag::AIWF);
        }
    }
    catch (const AVException& e) {
        std::cerr << "OutputStream::writeFrame exception: " << e.what() << std::endl;
    }

    return ret;
}

void OutputStream::close()
{
    avcodec_free_context(&enc);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

