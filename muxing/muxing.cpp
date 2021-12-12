/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

 /**
  * @file
  * libavformat API example.
  *
  * Output a media file in any supported libavformat format. The default
  * codecs are used.
  * @example muxing.c
  */

#include <queue>
#include "FileWriter.h"
#include "AudioGenerator.h"
#include "VideoGenerator.h"
#include "CircularQueue.h"

AVFrame* copyFrame(AVFrame* src)
{
    if (src == NULL)
        return NULL;

    AVFrame* dst = av_frame_alloc();
    dst->format = src->format;
    dst->channel_layout = src->channel_layout;
    dst->sample_rate = src->sample_rate;
    dst->nb_samples = src->nb_samples;
    dst->width = src->width;
    dst->height = src->height;
    av_frame_get_buffer(dst, 0);
    av_frame_make_writable(dst);
    av_frame_copy_props(dst, src);
    av_frame_copy(dst, src);
    return dst;
}

void video_frame_maker(VideoGenerator* generator, CircularQueue<AVFrame*>* q)
{
    bool running = true;
    while (running) {
        AVFrame* frame = copyFrame(generator->getFrame());
        q->push(frame);
        if (frame == NULL)
            running = false;
    }
}

void audio_frame_maker(AudioGenerator* generator, CircularQueue<AVFrame*>* q)
{
    bool running = true;
    while (running) {
        AVFrame* frame = copyFrame(generator->getFrame());
        q->push(frame);
        if (frame == NULL)
            running = false;
    }
}

void encoder(FileWriter* writer, CircularQueue<AVFrame*>* video_q, CircularQueue<AVFrame*>* audio_q)
{
    bool encoding = true;
    bool capturingVideo = true;
    bool capturingAudio = true;

    AVFrame* videoFrame = NULL;
    AVFrame* audioFrame = NULL;

    int video_frame_count = 0;
    int audio_frame_count = 0;

    while (encoding) {
        if (capturingVideo) {
            videoFrame = video_q->pop();
        }
        if (capturingAudio) {
            audioFrame = audio_q->pop();
        }

        int comparator = 1;
        if (videoFrame && audioFrame) {
            comparator = av_compare_ts(videoFrame->pts, writer->videoStream->enc->time_base,
                audioFrame->pts, writer->audioStream->enc->time_base);
        }

        if (comparator > 0) {
            if (audioFrame) {
                //std::cout << "audio write pts: " << audioFrame->pts << std::endl;
                writer->audioStream->writeFrame(audioFrame);
                av_frame_free(&audioFrame);
                audio_frame_count++;
                capturingVideo = false;
                capturingAudio = true;
            }
            else {
                capturingAudio = false;
            }
        }
        else {
            if (videoFrame) {
                //std::cout << "video write pts: " << videoFrame->pts << std::endl;
                writer->videoStream->writeFrame(videoFrame);
                av_frame_free(&videoFrame);
                video_frame_count++;
                capturingVideo = true;
                capturingAudio = false;
            }
            else {
                capturingVideo = false;
            }
        }

        if (!(capturingVideo || capturingAudio))
            encoding = false;
    }

    writer->videoStream->writeFrame(NULL);
    writer->audioStream->writeFrame(NULL);
    writer->close();

    std::cout << "video_frame_count: " << video_frame_count << std::endl;
    std::cout << "audio_frame_count: " << audio_frame_count << std::endl;
}


int main(int argc, char** argv)
{
    const char* filename = "test.mp4";
    StreamParameters params;
    params.pix_fmt = AV_PIX_FMT_YUV420P;
    params.width = 640;
    params.height = 480;
    params.frame_rate = 25;
    params.video_bit_rate = 400000;
    params.gop_size = 12;
    params.sample_fmt = AV_SAMPLE_FMT_FLTP;
    params.audio_bit_rate = 64000;
    params.sample_rate = 44100;
    params.channel_layout = AV_CH_LAYOUT_STEREO;

    FileWriter* writer = new FileWriter(filename, &params);
    AudioGenerator audioGenerator(writer->audioStream->enc);
    VideoGenerator videoGenerator(writer->videoStream->enc);

    CircularQueue<AVFrame*> video_q(10);
    CircularQueue<AVFrame*> audio_q(10);

    std::thread video_producer(video_frame_maker, &videoGenerator, &video_q);
    std::thread audio_producer(audio_frame_maker, &audioGenerator, &audio_q);
    std::thread file_writer(encoder, writer, &video_q, &audio_q);

    video_producer.join();
    audio_producer.join();
    file_writer.join();

    return 0;
}