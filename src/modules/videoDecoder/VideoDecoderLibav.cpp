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

bool VideoDecoderLibav::configDecoder(CodecType cType, PixType pType)
{
    AVCodecID codec_id;
    
    fCodec = cType;
    fPixType = pType;
    
    av_init_packet(&pkt);
    
    switch(fCodec){
        case H264:
            codec_id = AV_CODEC_ID_H264;
            break;
        case VP8:
            codec_id = AV_CODEC_ID_VP8;
            break;
        case MJPEG:
            codec_id = AV_CODEC_ID_MJPEG;
            break;
        case RAW:
            //TODO: set raw video codec
        default:
            //TODO: error
            return false;
            break;
    }
    
    frame = avcodec_alloc_frame();
    
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
    
    codecCtx->get_format = &this->getFormat;

    //TODO: check why is it this
    if (codec->capabilities & CODEC_CAP_TRUNCATED){
        codecCtx->flags |= CODEC_FLAG_TRUNCATED;
    }
    
    if (codec->capabilities & CODEC_CAP_SLICE_THREADS) {
	codecCtx->thread_count = 0; // == X264_THREADS_AUTO
        codecCtx->thread_type = FF_THREAD_SLICE;
    }
    //we can receive truncated frames
    codecCtx->flags2 |= CODEC_FLAG2_CHUNKS;

    AVDictionary* dictionary = nullptr;
    if (avcodec_open2(codecCtx, codec, &dictionary) < 0)
    {
        //TODO: error
        return false;
    }
    
    frame = avcodec_alloc_frame();
    
    return true;
}

bool VideoDecoderLibav::toBuffer(VideoFrame *decodedFrame)
{
    unsigned int length;
    
    if (frame->format == codecCtx->get_format()){
    
        length = avpicture_get_size(frame->dstFormat, codecCtx->width, 
                                codecCtx->height)*sizeof(uint8_t));  
    
        length = avpicture_layout((AVPicture *)frame, frame->format, codecCtx->width, codecCtx->height, decodedFrame->getFrame(), length);
        
        if (length <= 0){
            return false;
        }
    } else {
        return false;
    }
   
    decodedFrame->setFrameLength(length);
    
    return true;
}

bool VideoDecoderLibav::decodeFrame(VideoFrame *codedFrame, VideoFrame *decodedFrame)
{     
    int len, gotFrame;

    pkt.size = codedFrame->getLength();
    pkt.data = codedFrame->getFrame();
            
    while (pkt.size > 0) {
        len = avcodec_decode_video2(codecCtx, frame, &gotFrame, pkt);

        if(len < 0) {
            //TODO: error
            return false;
        }

        //TODO: something about B-frames?
        
        if (gotFrame){
            if (toBuffer(decodedFrame) <= 0){
                //TODO: error
                return false;
            }
        }
        
        if(pkt.data) {
            pkt.size -= len;
            pkt.data += len;
        }
    }
        
    return true;
}

int VideoDecoderLibav::getFormat(){
    int pix_fmt;
    
    switch(fPixType){
        case RGB24:
            pix_fmt = PIX_FMT_RGB24;
            break;
        case RGB32:
            pix_fmt = PIX_FMT_RGB32;
            break;
        case YUYV422:
            pix_fmt = PIX_FMT_YUYV422;
            break;
        default:
            //TODO: error
            pix_fmt = -1;
    }
    
    return pix_fmt;
}
