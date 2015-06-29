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

VideoResampler::VideoResampler(FilterRole fRole_, bool sharedFrames) : OneToOneFilter(true, fRole_, sharedFrames)
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

    initializeEventMap();
}

VideoResampler::~VideoResampler()
{
    av_free(inFrame);
    av_free(outFrame);
    sws_freeContext(imgConvertCtx);
}

FrameQueue* VideoResampler::allocQueue(int wId)
{
    return VideoFrameQueue::createNew(RAW, DEFAULT_RAW_VIDEO_FRAMES, outPixFmt);
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

        needsConfig = false;
    }
    return true;
}

bool VideoResampler::doProcessFrame(Frame *org, Frame *dst)
{
    int outWidth, outHeight;
    int height;

    VideoFrame* dstFrame = dynamic_cast<VideoFrame*>(dst);
    VideoFrame* orgFrame = dynamic_cast<VideoFrame*>(org);

    if (!reconfigure(orgFrame)){
        return false;
    }
    
    if (discartPeriod != 0 && discartCount < discartPeriod){
        discartCount++;
    } else if (discartPeriod != 0 && discartCount == discartPeriod){
        discartCount = 1;
        return false;
    }

    if (!setAVFrame(inFrame, orgFrame, libavInPixFmt)){
        return false;
    }

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
    
    dstFrame->setLength(avpicture_get_size (libavOutPixFmt, outWidth, outHeight));
    dstFrame->setSize(outWidth, outHeight);
    dstFrame->setPixelFormat(outPixFmt);

    if (!setAVFrame(outFrame, dstFrame, libavOutPixFmt)){
        return false;
    }
    
    height = sws_scale(imgConvertCtx, inFrame->data, inFrame->linesize, 0, 
              inFrame->height, outFrame->data, outFrame->linesize);
    
    if (height <= 0){
        utils::errorMsg("Could not convert image");
        return false;
    }
    
    dstFrame->setPresentationTime(orgFrame->getPresentationTime());
    dstFrame->setOriginTime(orgFrame->getOriginTime());

    dst->setConsumed(true);
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

bool VideoResampler::configEvent(Jzon::Node* params)
{
    int width, height, period;
    PixType pixelType;
       
    if (!params) {
        return false;
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
            return false;
        }
        pixelType = static_cast<PixType> (pixel);
    }

    return configure(width, height, period, pixelType);
}

void VideoResampler::initializeEventMap()
{
    eventMap["configure"] = std::bind(&VideoResampler::configEvent, this, std::placeholders::_1);
}

void VideoResampler::doGetState(Jzon::Object &filterNode)
{
    //TODO:
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
        case YUVJ420P:
            return AV_PIX_FMT_YUVJ420P;
            break;
        default:
            utils::errorMsg("[Resampler] Unknown output pixel format");
            break;
    }
    
    return AV_PIX_FMT_NONE;
}

bool VideoResampler::setAVFrame(AVFrame *aFrame, VideoFrame* vFrame, AVPixelFormat format)
{      
    if (avpicture_fill((AVPicture *) aFrame, vFrame->getDataBuf(), 
            format, vFrame->getWidth(), 
            vFrame->getHeight()) <= 0){
        utils::errorMsg("Could not feed AVFrame");
        return false;
    }
    
    aFrame->width = vFrame->getWidth();
    aFrame->height = vFrame->getHeight();
    aFrame->format = format;
    
    return true;
}

