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

PixType getPixelFormat(AVPixelFormat format);

VideoDecoderLibav::VideoDecoderLibav() : OneToOneFilter()
{
    avcodec_register_all();

    codecCtx = NULL;;
    frame = NULL;
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    fType = VIDEO_DECODER;

    outputStreamInfo = new StreamInfo (AUDIO);
    outputStreamInfo->video.codec = RAW;
    outputStreamInfo->video.pixelFormat = RGB24;

    frame = av_frame_alloc();
    frameCopy = av_frame_alloc();
    
    fCodec = VC_NONE;
}

VideoDecoderLibav::~VideoDecoderLibav()
{
    avcodec_close(codecCtx);
    av_free(codecCtx);
    av_free(frame);
    av_free(frameCopy);
    av_free_packet(&pkt);

    delete outputStreamInfo;
}


FrameQueue* VideoDecoderLibav::allocQueue(ConnectionData cData)
{
    return VideoFrameQueue::createNew(cData, outputStreamInfo, DEFAULT_RAW_VIDEO_FRAMES);
}

bool VideoDecoderLibav::doProcessFrame(Frame *org, Frame *dst)
{
    int len, gotFrame = 0;
    VideoFrame* vDecodedFrame = dynamic_cast<VideoFrame*>(dst);
    VideoFrame* vCodedFrame = dynamic_cast<VideoFrame*>(org);
    
    if (!reconfigure(vCodedFrame->getCodec())){
        return false;
    }
       
    pkt.size = org->getLength();
    pkt.data = org->getDataBuf();
   
    while (pkt.size > 0) {
        len = avcodec_decode_video2(codecCtx, frame, &gotFrame, &pkt);

        if(len <= 0) {
            utils::errorMsg("Decoding video frame");
            return false;
        }

        if (gotFrame) {
            if (toBuffer(vDecodedFrame, vCodedFrame)) {
                vDecodedFrame->setConsumed(true);
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
        case H265:
            libavCodecId = AV_CODEC_ID_HEVC;
            break;
        case VP8: 
            libavCodecId = AV_CODEC_ID_VP8;
            break;
        case MJPEG: //TODO
            libavCodecId = AV_CODEC_ID_MJPEG;
            break;
        case RAW:
            libavCodecId = AV_CODEC_ID_RAWVIDEO ;
        default:
            utils::errorMsg("Codec not supported");
            return false;
            break;
    }

    if (codecCtx != NULL) {
        avcodec_close(codecCtx);
        av_free(codecCtx);
    }

    codec = avcodec_find_decoder(libavCodecId);
    if (codec == NULL)
    {
        utils::errorMsg("Required codec not found");
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

    FrameQueue *in_queue = getReader(DEFAULT_ID)->getQueue();
    codecCtx->extradata = in_queue->getStreamInfo()->extradata;
    codecCtx->extradata_size = in_queue->getStreamInfo()->extradata_size;

    AVDictionary* dictionary = NULL;
    if (avcodec_open2(codecCtx, codec, &dictionary) < 0)
    {
        utils::errorMsg("Could not open required codec");
        return false;
    }

    return true;
}

bool VideoDecoderLibav::toBuffer(VideoFrame *decodedFrame, VideoFrame *codedFrame)
{
    int ret, length;
    
    length = avpicture_fill((AVPicture *) frameCopy, decodedFrame->getDataBuf(), 
                            (AVPixelFormat) frame->format, frame->width, frame->height); 
    if (length <= 0){
        utils::errorMsg("Could not fill decoded frame");
        return false;
    }
    
    frameCopy->width = frame->width;
    frameCopy->height = frame->height;
    frameCopy->format = frame->format;
       
    ret = av_frame_copy(frameCopy, frame);
    if (ret < 0){
        utils::errorMsg("Could not copy decoded frame data");
        return false;
    }
    
    decodedFrame->setLength(length);
    decodedFrame->setSize(frame->width, frame->height);
    decodedFrame->setPixelFormat(getPixelFormat((AVPixelFormat) frame->format));
    
    return true;
}



bool VideoDecoderLibav::reconfigure(VCodecType codec)
{
    if (fCodec == codec) {
        return true;
    }

    fCodec = codec;

    if(!inputConfig()) {
        utils::errorMsg("Configuring decoder");
        return false;
    }
    
    return true;
}

void VideoDecoderLibav::initializeEventMap()
{
	
}

void VideoDecoderLibav::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("codec", utils::getVideoCodecAsString(fCodec));
}

PixType getPixelFormat(AVPixelFormat format)
{
    switch(format){
        case AV_PIX_FMT_RGB24:
            return RGB24;
            break;
        case AV_PIX_FMT_RGB32:
            return RGB32;
            break;
        case AV_PIX_FMT_YUYV422:
            return YUYV422;
            break;
        case AV_PIX_FMT_YUV420P:
            return YUV420P;
            break;
        case AV_PIX_FMT_YUV422P:
            return YUV422P;
            break;
        case AV_PIX_FMT_YUV444P:
            return YUV444P;
            break;
        case AV_PIX_FMT_YUVJ420P:
            return YUVJ420P;
            break;
        default:
            utils::errorMsg("[Decoder] Unknown output pixel format");
            break;
    }
    
    return P_NONE;
}
