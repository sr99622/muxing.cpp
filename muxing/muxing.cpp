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

#include "FileWriter.h"
#include "AudioGenerator.h"
#include "VideoGenerator.h"

int main(int argc, char** argv)
{
    const char* filename;
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

    int i;

    if (argc < 2) {
        printf("usage: %s output_file\n"
            "API example program to output a media file with libavformat.\n"
            "This program generates a synthetic audio and video stream, encodes and\n"
            "muxes them into a file named output_file.\n"
            "The output format is automatically guessed according to the file extension.\n"
            "Raw images can also be output by using '%%d' in the filename.\n"
            "\n", argv[0]);
        return 1;
    }

    filename = argv[1];
    for (i = 2; i + 1 < argc; i += 2) {
        if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
            av_dict_set(&params.opts, argv[i] + 1, argv[i + 1], 0);
    }

    AVExceptionHandler av;

    try {
        FileWriter* writer = new FileWriter(filename, &params);
        AudioGenerator audioGenerator(writer->audioStream->enc);
        VideoGenerator videoGenerator(writer->videoStream->enc);

        bool encoding = true;
        bool capturingVideo = true;
        bool capturingAudio = true;

        AVFrame* videoFrame = NULL;
        AVFrame* audioFrame = NULL;

        while (encoding) {
            if (capturingVideo) 
                videoFrame = videoGenerator.getFrame();
            if (capturingAudio) 
                audioFrame = audioGenerator.getFrame();

            int comparator = 1;
            if (videoFrame && audioFrame) {
                comparator = av_compare_ts(videoFrame->pts, writer->videoStream->enc->time_base,
                    audioFrame->pts, writer->audioStream->enc->time_base);
            }

            if (comparator > 0) {
                if (audioFrame) {
                    writer->audioStream->writeFrame(audioFrame);
                    capturingVideo = false;
                    capturingAudio = true;
                }
                else {
                    capturingAudio = false;
                }
            }
            else {
                if (videoFrame) {
                    writer->videoStream->writeFrame(videoFrame);
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

    }
    catch (const AVException& e) {
        std::cerr << "main program exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}