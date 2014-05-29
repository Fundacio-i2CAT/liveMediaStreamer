/*
 *  VideoDecoderLibav.cpp - A libav-based video decoder
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

#include "VideoDecoderLibav.hh"
#include "../../AVFramedQueue.hh"

VideoDecoderLibav::VideoDecoderLibav()
{
    avcodec_register_all();

    codecCtx = NULL;;
    frame = NULL;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    //TODO: how to use callback?
    //av_log_set_callback(error_callback);
}

bool VideoDecoderLibav::configDecoder(VCodecType cType, PixType pType)
{   
    av_init_packet(&pkt);
    
    switch(cType){
        case H264:
            codec_id = AV_CODEC_ID_H264;
            break;
        case VP8: //TODO
            codec_id = AV_CODEC_ID_VP8;
            break;
        case MJPEG: //TODO
            codec_id = AV_CODEC_ID_MJPEG;
            break;
        case RAW:
            //TODO: set raw video codec
        default:
            //TODO: error
            return false;
            break;
    }
    
    switch(pType){
        case RGB24:
            pix_fmt = AV_PIX_FMT_RGB24;
            break;
        case RGB32:
            pix_fmt = AV_PIX_FMT_RGB32;
            break;
        case YUYV422:
            pix_fmt = AV_PIX_FMT_YUYV422;
            break;
        default:
            //TODO: error
            return false;
            break;
    }
     
    frame = av_frame_alloc();
    outFrame = av_frame_alloc();
    
    codec = avcodec_find_decoder(codec_id);
    if (codec == NULL)
    {
        //TODO: error
        return false;
    }
    
    codecCtx = avcodec_alloc_context3(codec);
    if (codecCtx == NULL) {
        return false;
    }

    if (codec->capabilities & CODEC_CAP_TRUNCATED){
        codecCtx->flags |= CODEC_FLAG_TRUNCATED;
    }
     
    if (codec->capabilities & CODEC_CAP_SLICE_THREADS) {
        codecCtx->thread_count = 0; 
        codecCtx->thread_type = FF_THREAD_SLICE;
    }

    codecCtx->flags2 |= CODEC_FLAG2_CHUNKS;

    AVDictionary* dictionary = NULL;
    if (avcodec_open2(codecCtx, codec, &dictionary) < 0)
    {
        //TODO: error
        return false;
    }
    
    return true;
}

bool VideoDecoderLibav::toBuffer(VideoFrame *decodedFrame)
{
    unsigned int length;
    
    if (!imgConvertCtx) {
        imgConvertCtx = sws_getContext(codecCtx->width, codecCtx->height, 
                                       (AVPixelFormat) frame->format, 
                                       codecCtx->width, codecCtx->height,
                                       pix_fmt, SWS_FAST_BILINEAR, 0, 0, 0);
        
        if (!imgConvertCtx){
            //TODO: error
            return false;
        }
    }
    
    length = avpicture_fill((AVPicture *) outFrame, decodedFrame->getDataBuf(), 
                   pix_fmt, codecCtx->width, codecCtx->height);
    
    if (length <= 0){
        return false;
    }
           
    if (frame->format == pix_fmt){
        length = avpicture_layout((AVPicture *)frame, pix_fmt,
                                  codecCtx->width, codecCtx->height, 
                                  decodedFrame->getDataBuf(), length);
        if (length <= 0){
            return false;
        }
    } else {
        sws_scale(imgConvertCtx, frame->data, frame->linesize, 0, 
                  codecCtx->height, outFrame->data, outFrame->linesize);
    }  
                          
    decodedFrame->setLength(length);
    decodedFrame->setSize(codecCtx->width, codecCtx->height);
    
    return true;
}

bool VideoDecoderLibav::doProcessFrame(Frame *org, Frame *dst)
{
    int len, gotFrame = 0;
    VideoFrame* vDecodedFrame = dynamic_cast<VideoFrame*>(dst);

    pkt.size = org->getLength();
    pkt.data = org->getDataBuf();
   
    while (pkt.size > 0) {
        len = avcodec_decode_video2(codecCtx, frame, &gotFrame, &pkt);

        if(len <= 0) {
            //TODO: error
            return false;
        }

        //TODO: something about B-frames?
        
        if (gotFrame){
            if (toBuffer(vDecodedFrame) <= 0){
                //TODO: error
                return false;
            }
        }
        
        if (pkt.data){
            pkt.size -= len;
            pkt.data += len;
        }
    }
        
    return true;
}

FrameQueue* VideoDecoderLibav::allocQueue(int wId)
{
    return VideoFrameQueue::createNew(RAW, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, RGB24);
}

