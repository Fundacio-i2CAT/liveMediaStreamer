/*
 *  VideoEncoderX264or5 - Base class for VideoEncoderX264 and VideoEncoderX265
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *            David Cassany <david.cassany@i2cat.net>
 */

#ifndef _VIDEO_ENCODER_X264_OR_5_HH
#define _VIDEO_ENCODER_X264_OR_5_HH

#include <stdint.h>
#include <chrono>
#include "../../Utils.hh"
#include "../../VideoFrame.hh"
#include "../../Filter.hh"
#include "../../FrameQueue.hh"
#include "../../Types.hh"
#include "../../StreamInfo.hh"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
}

#define DEFAULT_BITRATE 2000
#define DEFAULT_GOP 25
#define DEFAULT_LOOKAHEAD 25
#define DEFAULT_THREADS 4
#define DEFAULT_ANNEXB true
#define DEFAULT_B_FRAMES 4
#define DEFAULT_PRESET "ultrafast"

/*! Base class for VideoEncoderX264 and VideoEncoderX265. It implements common methods, basically configure and doProcessFrame */

class VideoEncoderX264or5 : public OneToOneFilter {
    
public:
    /**
    * Class constructor
    */
    VideoEncoderX264or5();

    /**
    * Class destructor
    */
    virtual ~VideoEncoderX264or5();

    bool configure(int bitrate, int fps, int gop, int lookahead, int bFrames, int threads, bool annexB, std::string preset);
    
protected:
    AVPixelFormat libavInPixFmt;
    AVFrame *midFrame;
    
    PixType inPixFmt;
    bool forceIntra;
    unsigned fps;
    unsigned bitrate;
    unsigned gop;
    unsigned threads;
    unsigned lookahead;
    unsigned bFrames;
    bool needsConfig;
    std::string preset;
    int64_t inPts;
    int64_t outPts;
    int64_t dts;

    StreamInfo *outputStreamInfo;
    
    bool doProcessFrame(Frame *org, Frame *dst);
    void initializeEventMap();      
    virtual bool fillPicturePlanes(unsigned char** data, int* linesize) = 0;
    virtual bool encodeFrame(VideoFrame* codedFrame) = 0;
    virtual bool reconfigure(VideoFrame* orgFrame, VideoFrame* dstFrame) = 0;
    void setIntra(){forceIntra = true;};
    bool fill_x264or5_picture(VideoFrame* videoFrame);

    bool configure0(unsigned bitrate_, unsigned fps_, unsigned gop_, unsigned lookahead_, unsigned bFrames_, unsigned threads_, bool annexB_, std::string preset_);
    
private:
    bool forceIntraEvent(Jzon::Node* params);
    bool configEvent(Jzon::Node* params);
    void doGetState(Jzon::Object &filterNode);
    
    //There is no need of specific reader configuration
    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};
    
    //NOTE: There is no need of specific writer configuration
    bool specificWriterConfig(int /*writerID*/) {return true;};
    bool specificWriterDelete(int /*writerID*/) {return true;};
    
    struct FrameTimeParams {
        std::chrono::microseconds pTime;
        std::chrono::system_clock::time_point oTime;
        size_t seqNum;
    };
    
    std::map<int64_t, FrameTimeParams> qFTP;
};

#endif
