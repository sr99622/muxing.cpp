/*******************************************************************************
* avexception.h
*
* Copyright (c) 2020 Stephen Rhodes
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*******************************************************************************/

#ifndef AVEXCEPTION_H
#define AVEXCEPTION_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <exception>
#include <iostream>

enum class CmdTag {
    NONE,
    AO2,
    AOI,
    ACI,
    AFSI,
    APTC,
    APFC,
    AWH,
    AWT,
    AO,
    AC,
    ACP,
    AAOC2,
    AFMW,
    AFGB,
    AHCC,
    AFBS,
    AWF,
    ASP,
    ASF,
    AEV2,
    ARF,
    ADV2,
    ARP,
    AIWF,
    AFE,
    AAC3,
    AFA,
    AAC,
    AFC,
    ABR,
    AHFTBN,
    AGHC,
    ANS,
    SGC,
    AFIF,
    APA,
    ADC,
    SA,
    SI,
    SC
};

class AVException : public std::exception
{
public:
    AVException(char* msg);

    const char* what() const throw () {
        return buffer;
    }
    char* buffer;
};

class AVExceptionHandler
{

public:
    void ck(int ret);
    void ck(int ret, CmdTag cmd_tag);
    void ck(AVFrame* arg, CmdTag cmd_tag = CmdTag::NONE);
    void ck(const AVCodec* arg, CmdTag cmd_tag = CmdTag::NONE);
    void ck(const AVCodec* arg, AVCodecID codec_id, CmdTag cmd_tag = CmdTag::NONE);
    void ck(AVPacket* arg, CmdTag cmd_tag = CmdTag::NONE);
    void ck(AVCodecContext* arg, CmdTag cmd_tag = CmdTag::NONE);
    void ck(AVStream* arg, CmdTag cmd_tag = CmdTag::NONE);
    void ck(SwrContext* arg, CmdTag cmd_tag = CmdTag::NONE);
    void ck(SwsContext* arg, CmdTag cmd_tag = CmdTag::NONE);
    const AVException getNullException(CmdTag cmd_tag);
    const char* tag(CmdTag cmd_tag);

};

#endif // AVEXCEPTION_H
