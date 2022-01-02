 /*
  * derived from @example muxing.c Copyright (c) 2003 Fabrice Bellard
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

void video_encoder(VideoStream* stream, CircularQueue<AVFrame*>* frame_q)
{
    bool running = true;
    while (running) {
        AVFrame* frame = frame_q->pop();
        stream->writeFrame(frame);
        if (frame == NULL) {
            running = false;
            stream->pkt_q->push(NULL);
        }
        else {
            av_frame_free(&frame);
        }
    }
}

void audio_encoder(AudioStream* stream, CircularQueue<AVFrame*>* frame_q)
{
    bool running = true;
    while (running) {
        AVFrame* frame = frame_q->pop();
        stream->writeFrame(frame);
        if (frame == NULL) {
            running = false;
            stream->pkt_q->push(NULL);
        }
        else {
            av_frame_free(&frame);
        }
    }
}

void video_audio_handler(FileWriter* writer, CircularQueue<AVPacket*>* video_pkt_q, CircularQueue<AVPacket*>* audio_pkt_q)
{
    bool encoding = true;
    bool capturingVideo = false;
    if (video_pkt_q)
        capturingVideo = true;
    bool capturingAudio = false;
    if (audio_pkt_q)
        capturingAudio = true;

    AVPacket* videoPkt = NULL;
    AVPacket* audioPkt = NULL;

    int video_frame_count = 0;
    int audio_frame_count = 0;

    while (encoding) {
        if (capturingVideo) {
            try {
                videoPkt = video_pkt_q->pop();
                if (!videoPkt) {
                    capturingVideo = false;
                }
            }
            catch (const QueueClosedException& e) {
                std::cout << "video_pkt_q exception: " << e.what() << std::endl;
            }
        }
        if (capturingAudio) {
            try {
                audioPkt = audio_pkt_q->pop();
                if (!audioPkt) {
                    capturingAudio = false;
                }
            }
            catch (const QueueClosedException& e) {
                std::cout << "audio_pkt_q exception: " << e.what() << std::endl;
            }
        }

        int comparator = 1;
        if (videoPkt && audioPkt) {
            //comparator = av_compare_ts(videoFrame->pts, videoStream->enc_ctx->time_base,
            //    audioFrame->pts, audioStream->enc->time_base);
            if (videoPkt->pts > audioPkt->pts)
                comparator = 1;
            else
                comparator = -1;
        }
        else {
            if (videoPkt)
                comparator = -1;
        }

        if (comparator > 0) {
            if (audioPkt) {
                //std::cout << "audio write pts: " << audioFrame->pts << std::endl;
                writer->writeFrame(audioPkt);
                av_packet_free(&audioPkt);
                audio_frame_count++;
                capturingVideo = false;
                capturingAudio = true;
            }
            else {
                capturingAudio = false;
            }
        }
        else {
            if (videoPkt) {
                //std::cout << "video write pts: " << videoFrame->pts << std::endl;
                writer->writeFrame(videoPkt);
                av_packet_free(&videoPkt);
                video_frame_count++;
                capturingVideo = true;
                capturingAudio = false;
            }
            else {
                capturingVideo = false;
            }
        }

        if (!(capturingVideo || capturingAudio)) {
            encoding = false;
        }
    }
    std::cout << "video_frame_count: " << video_frame_count << std::endl;
    std::cout << "audio_frame_count: " << audio_frame_count << std::endl;
}

int main(int argc, char** argv)
{
    const char* filename = "test.mp4";
    StreamParameters params;
    params.format = "mp4";
    params.pix_fmt = AV_PIX_FMT_YUV420P;
    params.width = 640;
    params.height = 480;
    params.frame_rate = 25;
    params.video_time_base = av_make_q(1, params.frame_rate);
    params.video_bit_rate = 600000;
    params.gop_size = 12;
    params.sample_fmt = AV_SAMPLE_FMT_FLTP;
    params.audio_bit_rate = 64000;
    params.sample_rate = 44100;
    params.audio_time_base = av_make_q(1, params.sample_rate);
    params.channel_layout = AV_CH_LAYOUT_STEREO;
    params.channels = 2;
    params.nb_samples = 1024;
    params.audio_codec_name = "aac";
    params.hw_device_type = AV_HWDEVICE_TYPE_CUDA;
    params.hw_pix_fmt = AV_PIX_FMT_CUDA;
    params.sw_pix_fmt = AV_PIX_FMT_YUV420P;
    params.video_codec_name = "h264_nvenc";
    params.profile = "high";

    VideoGenerator videoGenerator(params);
    AudioGenerator audioGenerator(params);

    FileWriter writer(params);

    CircularQueue<AVPacket*> video_pkt_q(10);
    CircularQueue<AVPacket*> audio_pkt_q(10);

    VideoStream videoStream(&writer, params, &video_pkt_q);
    AudioStream audioStream(&writer, params, &audio_pkt_q);

    CircularQueue<AVFrame*> video_frame_q(10);
    CircularQueue<AVFrame*> audio_frame_q(10);
    CircularQueue<AVFrame*>* ptr = NULL;

    writer.openFile(filename);

    std::thread video_producer(video_frame_maker, &videoGenerator, &video_frame_q);
    std::thread audio_producer(audio_frame_maker, &audioGenerator, &audio_frame_q);
    std::thread video_encoding(video_encoder, &videoStream, &video_frame_q);
    std::thread audio_encoding(audio_encoder, &audioStream, &audio_frame_q);
    std::thread file_writing(video_audio_handler, &writer, &video_pkt_q, &audio_pkt_q);

    video_producer.join();
    //video_frame_q.flush();
    audio_producer.join();
    //audio_frame_q.flush();
    video_encoding.join();
    audio_encoding.join();
    file_writing.join();
    
    writer.closeFile();

    return 0;
}






/*
void encoder(FileWriter* writer, VideoStream* videoStream, CircularQueue<AVFrame*>* video_q,
    AudioStream* audioStream, CircularQueue<AVFrame*>* audio_q)
{
    bool encoding = true;
    bool capturingVideo = false;
    if (video_q)
        capturingVideo = true;
    bool capturingAudio = false;
    if (audio_q)
        capturingAudio = true;

    AVFrame* videoFrame = NULL;
    AVFrame* audioFrame = NULL;

    int video_frame_count = 0;
    int audio_frame_count = 0;

    while (encoding) {
        if (capturingVideo) {
            try {
                videoFrame = video_q->pop();
                if (!videoFrame) {
                    capturingVideo = false;
                    std::cout << "video - got null ptr eof" << std::endl;
                }
            }
            catch (const QueueClosedException& e) {
                std::cout << "video_q exception: " << e.what() << std::endl;
            }
        }
        if (capturingAudio) {
            try {
                audioFrame = audio_q->pop();
                if (!audioFrame) {
                    capturingAudio = false;
                    std::cout << "audio - got null ptr eof" << std::endl;
                }
            }
            catch (const QueueClosedException& e) {
                std::cout << "audio_q exception: " << e.what() << std::endl;
            }
        }

        int comparator = 1;
        if (videoFrame && audioFrame) {
            comparator = av_compare_ts(videoFrame->pts, videoStream->enc_ctx->time_base,
                audioFrame->pts, audioStream->enc->time_base);
        }
        else {
            if (videoFrame)
                comparator = -1;
        }

        if (comparator > 0) {
            if (audioFrame) {
                //std::cout << "audio write pts: " << audioFrame->pts << std::endl;
                audioStream->writeFrame(audioFrame);
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
                videoStream->writeFrame(videoFrame);
                av_frame_free(&videoFrame);
                video_frame_count++;
                capturingVideo = true;
                capturingAudio = false;
            }
            else {
                capturingVideo = false;
            }
        }

        if (!(capturingVideo || capturingAudio)) {
            std::cout << "EOF" << std::endl;
            encoding = false;
        }
    }

    videoStream->writeFrame(NULL);
    audioStream->writeFrame(NULL);

    std::cout << "video_frame_count: " << video_frame_count << std::endl;
    std::cout << "audio_frame_count: " << audio_frame_count << std::endl;
}
*/
