#include "FileWriter.h"

FileWriter::FileWriter(const char* filename, StreamParameters* params)
{
    this->params = params;

    try {
        av.ck(avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename), CmdTag::AAOC2);
        if (fmt_ctx->oformat->video_codec != AV_CODEC_ID_NONE) {
            //videoStream = new VideoStream(fmt_ctx, opts);
            videoStream = new VideoStream(this);
        }
        if (fmt_ctx->oformat->audio_codec != AV_CODEC_ID_NONE) {
            //audioStream = new AudioStream(fmt_ctx, params->opts);
            audioStream = new AudioStream(this);
        }

        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
            av.ck(avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE), CmdTag::AO);

        av.ck(avformat_write_header(fmt_ctx, &params->opts), CmdTag::AWH);

    }
    catch (const AVException& e) {
        std::cerr << "FileWriter constructor exception: " << e.what() << std::endl;
    }
}

void FileWriter::close()
{
    try {
        av.ck(av_write_trailer(fmt_ctx), CmdTag::AWT);
        if (videoStream) {
            videoStream->close();
        }
        if (audioStream) {
            audioStream->close();
        }

        if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
            av.ck(avio_closep(&fmt_ctx->pb), CmdTag::AC);

        avformat_free_context(fmt_ctx);
        av_dict_free(&params->opts);
    }
    catch (const AVException& e) {
        std::cerr << "FileWriter::close exception: " << e.what() << std::endl;
    }
}


