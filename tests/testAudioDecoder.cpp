
#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS 1 
#endif

extern "C" {
    #include <libavformat/avformat.h>
}

#include "../src/AudioCircularBuffer.hh"
#include "../src/InterleavedAudioFrame.hh"
#include "../src/PlanarAudioFrame.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"

#include <iostream>
#include <thread>
#include <unistd.h>

bool should_stop = false;

struct buffer {
  uint8_t* data[8];
  int data_len[8];
};

void fillBuffer(struct buffer *b, Frame *pFrame, int channels, int channel_data_len) {
    int i=0, j=0, counter=0;

    for (i=0; i<channels; i++) {
        memcpy(b->data[i] + b->data_len[i], pFrame->getPlanarDataBuf()[i], channel_data_len);
        b->data_len[i] += channel_data_len; 
    }
}

void saveBuffer(struct buffer *b) {
    int i=0;

    for (i=0; i<2; i++) {
        FILE *audioChannel = NULL;
        char filename[32];
  
        sprintf(filename, "channel%d.pcm", i);

        audioChannel = fopen(filename, "wb");

        if (b->data[i] != NULL) {
            fwrite(b->data[i], b->data_len[i], 1, audioChannel);
        }
    
        fclose(audioChannel);
    }
}

void readingRoutine(struct buffer* b, AudioCircularBuffer* cb, AudioFrame* fr)
{
    fr->setSamples(512);
    fr->setLength(fr->getSamples()*fr->getBytesPerSample());

    while(!should_stop) {
        if(!cb->popFront(fr->getPlanarDataBuf(), fr->getSamples())) {
           // printf("POP failed\n");
            usleep(100);
            continue;
        }

        fillBuffer(b, fr, fr->getChannels(), fr->getLength());
    }

    saveBuffer(b);
    printf("Buffer saved\n");
}

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    int             i, audioStream;
    AVPacket        packet;
    int             frameFinished;
    AVDictionary    *optionsDict = NULL;
    struct buffer   *buffers;

    CodecType cType = MULAW;
    SampleFmt inSFmt = S16;
    SampleFmt outSFmt = S32P;
    unsigned int inCh = 2;
    unsigned int outCh = 2;
    unsigned int inSRate = 48000;
    unsigned int outSRate = 8000;
    unsigned int chMaxSamples = 3000;
    unsigned int bytesPerSample = 4;

    if(argc < 2) {
        printf("Please provide a movie file\n");
        return -1;
    }

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0 ) {
        return -1; // Couldn't open file
    }


    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    // Find the first video stream
    audioStream = -1;
    for(i=0; i<pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO) {
            audioStream=i;
            break;
        }
    }

    if(audioStream == -1) {
        return -1; // Didn't find a video stream
    }

    buffers = new struct buffer;

    for (i=0; i<2; i++) {
        buffers->data[i] = (uint8_t*)malloc(chMaxSamples * bytesPerSample * outSRate * 360);
        buffers->data_len[i] = 0;
    }

    AudioDecoderLibav* audioDecoder = new AudioDecoderLibav(cType, inSFmt, inCh, inSRate, outSFmt, outCh, outSRate);
    AudioCircularBuffer* audioCirBuffer = new AudioCircularBuffer(outCh, chMaxSamples, bytesPerSample);
    PlanarAudioFrame* aFrame = new PlanarAudioFrame(outCh, outSRate, chMaxSamples, cType, outSFmt);
    InterleavedAudioFrame* codedFrame = new InterleavedAudioFrame(inCh, inSRate, chMaxSamples, cType, inSFmt);
    PlanarAudioFrame* destinationPlanarFrame = new PlanarAudioFrame(outCh, outSRate, chMaxSamples, cType, outSFmt);

    std::thread readingThread(readingRoutine, buffers, audioCirBuffer, destinationPlanarFrame);  

    i=0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
        if(packet.stream_index == audioStream) {

            memcpy(codedFrame->getDataBuf(), packet.data, packet.size);
            codedFrame->setLength(packet.size);

            if(!audioDecoder->decodeFrame(codedFrame, aFrame)) {
                return -1;
            }

            while(!audioCirBuffer->pushBack(aFrame->getPlanarDataBuf(), aFrame->getSamples())) {
                //printf("Push back failed! Trying again\n");
                usleep(100);
            }

            //fillBuffer(buffers, aFrame, outCh, aFrame->getLength());
        }
    }

    should_stop = true;
    readingThread.join();

    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);

    //saveBuffer(buffers);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}