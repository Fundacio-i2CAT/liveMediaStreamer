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
#include "../../Utils.hh"

VideoDecoderLibav::VideoDecoderLibav()
{
    avcodec_register_all();

    codecCtx = NULL;;
    frame = NULL;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    fType = VIDEO_DECODER;

    frame = av_frame_alloc();
    outFrame = av_frame_alloc();

    imgConvertCtx = NULL;

    outputWidth = 0;
    outputHeight = 0;
    outputPixelFormat = RGB24;
    libavPixFmt = AV_PIX_FMT_RGB24;

    fCodec = VC_NONE;

    needsConfig = false;

    //TODO: how to use callback?
    //av_log_set_callback(error_callback);
}

FrameQueue* VideoDecoderLibav::allocQueue(int wId)
{
    return VideoFrameQueue::createNew(RAW, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, RGB24);
}

bool VideoDecoderLibav::doProcessFrame(Frame *org, Frame *dst)
{
    int len, gotFrame = 0;
    VideoFrame* vDecodedFrame = dynamic_cast<VideoFrame*>(dst);

    checkInputParams(dynamic_cast<VideoFrame*>(org)->getCodec());

    if(needsConfig) {
        outputConfig();
    }

    pkt.size = org->getLength();
    pkt.data = org->getDataBuf();
   
    while (pkt.size > 0) {
        len = avcodec_decode_video2(codecCtx, frame, &gotFrame, &pkt);

        if(len <= 0) {
            utils::errorMsg("Decoding video frame");
            return false;
        }

        //TODO: something about B-frames?
        
        if (gotFrame) {
            if (toBuffer(vDecodedFrame)) {
                return true;
            }
        }
        
        if (pkt.data){
            pkt.size -= len;
            pkt.data += len;
        }
    }
   
    return false;
}

bool VideoDecoderLibav::inputConfig()
{   
    switch(fCodec){
        case H264:
            libavCodecId = AV_CODEC_ID_H264;
            break;
        case VP8: //TODO
            libavCodecId = AV_CODEC_ID_VP8;
            break;
        case MJPEG: //TODO
            libavCodecId = AV_CODEC_ID_MJPEG;
            break;
        case RAW:
            //TODO: set raw video codec
        default:
            //TODO: error
            return false;
            break;
    }

    av_frame_unref(frame);
    av_frame_unref(outFrame);

    if (codecCtx != NULL) {
        avcodec_close(codecCtx);
        av_free(codecCtx);
    }

    codec = avcodec_find_decoder(libavCodecId);
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

bool VideoDecoderLibav::configure(int width, int height, PixType pixelFormat) {

    outputWidth = width;
    outputHeight = height;
    outputPixelFormat = pixelFormat;

    switch(outputPixelFormat){
        case RGB24:
            libavPixFmt = AV_PIX_FMT_RGB24;
            break;
        case RGB32:
            libavPixFmt = AV_PIX_FMT_RGB32;
            break;
        case YUYV422:
            libavPixFmt = AV_PIX_FMT_YUYV422;
            break;
        default:
            //TODO: error
            return false;
            break;
    }

    needsConfig = true;
}

bool VideoDecoderLibav::toBuffer(VideoFrame *decodedFrame)
{
    unsigned int length;



    if (outputWidth == 0 || outputHeight == 0) {
        outputWidth = codecCtx->width;
        outputHeight = codecCtx->height;
    }
    
    if (!imgConvertCtx) {
        imgConvertCtx = sws_getContext(codecCtx->width, codecCtx->height, 
                                        (AVPixelFormat) frame->format, 
                                        outputWidth, outputHeight,
                                        libavPixFmt, SWS_FAST_BILINEAR, 0, 0, 0);
        
        if (!imgConvertCtx){
            //TODO: error
            return false;
        }
    }
    
    length = avpicture_fill((AVPicture *) outFrame, decodedFrame->getDataBuf(), 
                   libavPixFmt, outputWidth, outputHeight);
    
    if (length <= 0){
        return false;
    }

    // if (frame->format == outputPixelFormat) {
    //     length = avpicture_layout((AVPicture *)frame, pix_fmt,
    //                               outputWidth, outputHeight, 
    //                               decodedFrame->getDataBuf(), length);
    //     if (length <= 0){
    //         return false;
    //     }

    // } else {
        sws_scale(imgConvertCtx, frame->data, frame->linesize, 0, 
                  codecCtx->height, outFrame->data, outFrame->linesize);
    // }  
                          
    decodedFrame->setLength(length);
    decodedFrame->setSize(outputWidth, outputHeight);
    
    return true;
}



void VideoDecoderLibav::checkInputParams(VCodecType codec)
{
    if (fCodec == codec) {
        return;
    }

    fCodec = codec;

    if(!inputConfig()) {
        utils::errorMsg("Configuring decoder");
    }
}

void VideoDecoderLibav::outputConfig()
{
    if (imgConvertCtx) {
        sws_freeContext(imgConvertCtx);  
    }

    needsConfig = false;
}

void VideoDecoderLibav::resizeEvent(Jzon::Node* params)
{
	if (!params) {
		return;
	}

	if (!params->Has("width") || !params->Has("height")) {
		return;
	}

	int width = params->Get("width").ToInt();
	int height = params->Get("height").ToInt();

	if (width)
		outputWidth = width;

	if (height)
		outputHeight = height;
	
	needsConfig = true;
}
void VideoDecoderLibav::changePixelFormatEvent(Jzon::Node* params)
{
	if (!params) {
		return;
	}

	if (!params->Has("pixelFormat")) {
		return;
	}
	
	int pixel = params->Get("pixelFormat").ToInt();
	if ((pixel < -1) || (pixel > 2)) {
		return;
	}

	PixType pixelType = static_cast<PixType> (pixel);
	
	switch(pixelType){
        case RGB24:
            libavPixFmt = AV_PIX_FMT_RGB24;
            break;
        case RGB32:
            libavPixFmt = AV_PIX_FMT_RGB32;
            break;
        case YUYV422:
            libavPixFmt = AV_PIX_FMT_YUYV422;
            break;
        default:
            return;
            break;
    }
	
	needsConfig = true;
}

void VideoDecoderLibav::initializeEventMap()
{
	eventMap["resize"] = std::bind(&VideoDecoderLibav::resizeEvent, this, std::placeholders::_1);
	eventMap["changePixelFormat"] = std::bind(&VideoDecoderLibav::changePixelFormatEvent, this, std::placeholders::_1);
}

void VideoDecoderLibav::doGetState(Jzon::Object &filterNode)
{
   /* filterNode.Add("codec", utils::getAudioCodecAsString(fCodec));
    filterNode.Add("sampleRate", sampleRate);
    filterNode.Add("channels", channels);
    filterNode.Add("sampleFormat", utils::getSampleFormatAsString(sampleFmt));*/
}

