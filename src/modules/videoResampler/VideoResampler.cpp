/*
 *  VideoResampler.cpp - A libav-based video decoder
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
 *  Authors: David Cassany <david.cassany@i2cat.net>  
 *           Marc Palau <marc.palau@i2cat.net>
 */

#include "VideoResampler.hh"
#include "../../AVFramedQueue.hh"
#include "../../Utils.hh"



AVPixelFormat getLibavPixFmt(PixType pixType);

VideoResampler::VideoResampler()
{
    fType = VIDEO_RESAMPLER;

    inFrame = av_frame_alloc();
    outFrame = av_frame_alloc();

    imgConvertCtx = NULL;

    outputWidth = 0;
    outputHeight = 0;
    discartPeriod = 0;
    discartCount = 1;
    outPixFmt = RGB24;
    libavOutPixFmt = getLibavPixFmt(outPixFmt);

    needsConfig = false;
}

FrameQueue* VideoResampler::allocQueue(int wId)
{
    return VideoFrameQueue::createNew(RAW, 0, outPixFmt);
}

bool VideoResampler::reconfigure(VideoFrame* orgFrame)
{      
    if (!imgConvertCtx || needsConfig || 
        orgFrame->getWidth() != inFrame->width ||
        orgFrame->getHeight() != inFrame->height ||
        orgFrame->getPixelFormat() != inPixFmt)
    {
        inPixFmt = orgFrame->getPixelFormat();
        libavInPixFmt = getLibavPixFmt(inPixFmt);

        if (libavInPixFmt == AV_PIX_FMT_NONE){
            return false;
        }
        
        int outWidth, outHeight;
        if (outputWidth == 0){
            outWidth = orgFrame->getWidth();
        } else {
            outWidth = outputWidth;
        }
        
        if (outputHeight == 0){
            outHeight = orgFrame->getHeight();
        } else {
            outHeight = outputHeight;
        }
        
        imgConvertCtx = sws_getContext(orgFrame->getWidth(), orgFrame->getHeight(), 
                                       libavInPixFmt, outWidth, outHeight,
                                       libavOutPixFmt, SWS_FAST_BILINEAR, 0, 0, 0);
        if (!imgConvertCtx){
            utils::errorMsg("Could not get the swscale context");
            return false;
        }
    }
    return true;
}

bool VideoResampler::doProcessFrame(Frame *org, Frame *dst)
{
    VideoFrame* dstFrame = dynamic_cast<VideoFrame*>(dst);
    VideoFrame* orgFrame = dynamic_cast<VideoFrame*>(org);
    int outWidth, outHeight;
    int length, height;

    if (!reconfigure(orgFrame)){
        return false;
    }
    
    if (discartPeriod != 0 && discartCount < discartPeriod){
        discartCount++;
    } else if (discartPeriod != 0 && discartCount == discartPeriod){
        discartCount = 1;
        return false;
    }

    length = avpicture_fill((AVPicture *) inFrame, orgFrame->getDataBuf(), 
                            libavInPixFmt, orgFrame->getWidth(), 
                            orgFrame->getHeight());
    
    if (length <= 0){
        utils::errorMsg("Could not read input frame");
        return false;
    }
    
    //NOTE: to delete it!
    if (! rawFramesIn.is_open()){
        rawFramesIn.open("framesIn.yuv", std::ios::out | std::ios::app | std::ios::binary);
    } 
    if (orgFrame->getLength() > 0) {
        rawFramesIn.write(reinterpret_cast<const char*>(orgFrame->getDataBuf()), orgFrame->getLength());
    }
    
    
    if (buff == NULL){
        buff = (unsigned char *) malloc(sizeof(char)* orgFrame->getLength());
    }
        
    length = avpicture_layout((AVPicture *)inFrame, (AVPixelFormat) inFrame->format,
                              inFrame->width, inFrame->height, 
                              buff, length);
    
    if (! rawFramesOut.is_open()){
        rawFramesOut.open("framesOut.yuv", std::ios::out | std::ios::app | std::ios::binary);
    } 
    if (length > 0) {
        rawFramesOut.write(reinterpret_cast<const char*>(buff), length);
    } else {
        utils::errorMsg("avpicture_layout failed");
    }
    //NOTE: finnish deletion part
    
    if (outputWidth == 0){
        outWidth = orgFrame->getWidth();
    } else {
        outWidth = outputWidth;
    }
    
    if (outputHeight == 0){
        outHeight = orgFrame->getHeight();
    } else {
        outHeight = outputHeight;
    }
    
    
    
    length = avpicture_fill((AVPicture *) outFrame, dstFrame->getDataBuf(), 
                            libavOutPixFmt, outWidth, outHeight);
    
    if (length <= 0){
        utils::errorMsg("Could not prepare output buffers");
        return false;
    }
    
    height = sws_scale(imgConvertCtx, inFrame->data, inFrame->linesize, 0, 
              inFrame->height, outFrame->data, outFrame->linesize);
    
    if (height <= 0){
        utils::errorMsg("Could not convert image");
        return false;
    }
    
    dstFrame->setLength(length);
    dstFrame->setSize(outWidth, outHeight);
    dstFrame->setPresentationTime(orgFrame->getPresentationTime());
    dstFrame->setUpdatedTime();
   
    return true;
}


bool VideoResampler::configure(int width, int height, int period, PixType pixelFormat) {

    outputWidth = width;
    outputHeight = height;
    outPixFmt = pixelFormat;
    discartPeriod = period;
    
    libavOutPixFmt = getLibavPixFmt(outPixFmt);
    needsConfig = true;
    
    if (libavOutPixFmt == AV_PIX_FMT_NONE){
        return false;
    }
    
    return true;
}

void VideoResampler::configEvent(Jzon::Node* params, Jzon::Object &outputNode)
{
    int width, height, period;
    PixType pixelType;
       
    if (!params) {
        return;
    }
    
    width = outputWidth;
    height = outputHeight;
    period = discartPeriod;
    pixelType = outPixFmt;
    
    if (params->Has("width")){
        width = params->Get("width").ToInt();
    }
    
    if (params->Has("height")){
        height = params->Get("height").ToInt();
    }
    
    if (params->Has("discartPeriod")){
        period = params->Get("discartPeriod").ToInt();
    }
    
    if (params->Has("pixelFormat")){
        int pixel = params->Get("pixelFormat").ToInt();
        if ((pixel < P_NONE) || (pixel > YUYV422)) {
            return;
        }
        pixelType = static_cast<PixType> (pixel);
    }
    
    
    if (!configure(width, height, period, pixelType)){
        outputNode.Add("error", "Error configuring audio decoder");
    } else {
        outputNode.Add("error", Jzon::null);
    }
}

void VideoResampler::initializeEventMap()
{
    eventMap["configEvent"] = std::bind(&VideoResampler::configEvent, this, std::placeholders::_1, std::placeholders::_2);
}

void VideoResampler::doGetState(Jzon::Object &filterNode)
{
    /* filterNode.Add("codec", utils::getAudioCodecAsString(fCodec));
     *   filterNode.Add("sampleRate", sampleRate);
     *   filterNode.Add("channels", channels);
     *   filterNode.Add("sampleFormat", utils::getSampleFormatAsString(sampleFmt));*/
}

AVPixelFormat getLibavPixFmt(PixType pixType)
{
    switch(pixType){
        case RGB24:
            return AV_PIX_FMT_RGB24;
            break;
        case RGB32:
            return AV_PIX_FMT_RGB32;
            break;
        case YUYV422:
            return AV_PIX_FMT_YUYV422;
            break;
        case YUV420P:
            return AV_PIX_FMT_YUV420P;
            break;
        case YUV422P:
            return AV_PIX_FMT_YUV422P;
            break;
        case YUV444P:
            return AV_PIX_FMT_YUV444P;
            break;
        default:
            utils::errorMsg("Unknown output pixel format");
            break;
    }
    
    return AV_PIX_FMT_NONE;
}
