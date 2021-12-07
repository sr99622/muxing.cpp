#include "VideoGenerator.h"

VideoGenerator::VideoGenerator(AVCodecContext* enc)
{
    next_pts = 0;
    width = enc->width;
    height = enc->height;
    pix_fmt = enc->pix_fmt;
    time_base = enc->time_base;

    try {
        av.ck(frame = VideoStream::allocateFrame(enc));
        av.ck(av_frame_make_writable(frame), CmdTag::AFMW);
        av.ck(yuv_frame = VideoStream::allocateFrame(AV_PIX_FMT_YUV420P, width, height));
        av.ck(av_frame_make_writable(yuv_frame), CmdTag::AFMW);
    }
    catch (const AVException& e) {
        std::cerr << "VideoGenerator constructor exception: " << e.what() << std::endl;
    }
}

VideoGenerator::~VideoGenerator()
{
    av_frame_free(&frame);
    av_frame_free(&yuv_frame);
    sws_freeContext(sws_ctx);
}

void VideoGenerator::fillFrame(AVFrame* pict, int frame_index)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

AVFrame* VideoGenerator::getFrame()
{
    if (av_compare_ts(next_pts, time_base, STREAM_DURATION, av_make_q(1, 1)) > 0)
        return NULL;

    try {
        if (pix_fmt != AV_PIX_FMT_YUV420P) {
            if (sws_ctx == NULL) {
                av.ck(sws_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height,
                    pix_fmt, SWS_BICUBIC, NULL, NULL, NULL), CmdTag::SGC);
                fillFrame(yuv_frame, next_pts);
                sws_scale(sws_ctx, (const uint8_t* const*)yuv_frame->data,
                    yuv_frame->linesize, 0, height, frame->data, frame->linesize);
            }
        }
        else {
            fillFrame(frame, next_pts);
        }
    }
    catch (const AVException& e) {
        std::cerr << "VideoGenerator::getFrame exception: " << e.what() << std::endl;
        return NULL;
    }

    frame->pts = next_pts++;
    return frame;
}
