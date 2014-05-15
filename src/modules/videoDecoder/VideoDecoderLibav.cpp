#include "VideoDecoderLibav.hh"

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

bool VideoDecoderLibav::decodeFrame(Frame *codedFrame, Frame *decodedFrame)
{
    int len, gotFrame = 0;
    VideoFrame* vDecodedFrame = dynamic_cast<VideoFrame*>(decodedFrame);

    pkt.size = codedFrame->getLength();
    pkt.data = codedFrame->getDataBuf();
   
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

