#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#include <string>

#ifndef _HANDLERS_HH
#include "../src/network/Handlers.hh"
#endif

#ifndef _SOURCE_MANAGER_HH
#include "../src/network/SourceManager.hh"
#endif

#include "../src/PlanarAudioFrame.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/AudioCircularBuffer.hh"

#include <iostream>
#include <csignal>


#define PROTOCOL "RTP"
#define PAYLOAD 97
#define BANDWITH 5000

#define A_CODEC "opus"
#define A_CLIENT_PORT 6006
#define A_MEDIUM "audio"
#define A_TIME_STMP_FREQ 48000
#define A_CHANNELS 2

bool should_stop = false;

struct buffer {
    uint8_t* data[8];
    int data_len[8];
};

void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    SourceManager *mngr = SourceManager::getInstance();
    mngr->closeManager();
    
    std::cout << "Manager closed\n";
}

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

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    Session* session;
    SourceManager *mngr = SourceManager::getInstance();
    AudioDecoderLibav* audioDecoder;
    std::map<unsigned short, FrameQueue*> inputs;
    FrameQueue* q;
    AudioCircularBuffer* audioCirBuffer;
    PlanarAudioFrame* aFrame;
    PlanarAudioFrame* destinationPlanarFrame;
    Frame *codedFrame;
    struct buffer *buffers;

    CodecType inCType = OPUS_C;
    CodecType outCType = PCM;
    SampleFmt inSFmt = S16;
    unsigned int inCh = 2;
    unsigned int inSRate = 48000;
    SampleFmt outSFmt = S16P;
    unsigned int outCh = 2;
    unsigned int outSRate = 48000;
    unsigned int chMaxSamples = 3000;
    unsigned int bytesPerSample = 2;

    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = handlers::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i]);
        mngr->addSession(sessionId, session);
    }
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    
    sdp = handlers::makeSessionSDP("testSession", "this is a test");
    
    sdp += handlers::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD, A_CODEC, BANDWITH, 
                                        A_TIME_STMP_FREQ, A_CLIENT_PORT, A_CHANNELS);
    
    session = Session::createNew(*(mngr->envir()), sdp);
    
    mngr->addSession(sessionId, session);
       
    mngr->runManager();
       
    mngr->initiateAll();

    audioDecoder = new AudioDecoderLibav();
    audioDecoder->configure(inCType, inSFmt, inCh, inSRate, outSFmt, outCh, outSRate);
    audioCirBuffer = new AudioCircularBuffer(outCh, chMaxSamples, bytesPerSample);
    aFrame = new PlanarAudioFrame(outCh, outSRate, chMaxSamples, outCType, outSFmt);
    destinationPlanarFrame = new PlanarAudioFrame(outCh, outSRate, chMaxSamples, outCType, outSFmt);

    inputs = mngr->getInputs();
    q = inputs[A_CLIENT_PORT];
    buffers = new struct buffer;

    for (int i=0; i<2; i++) {
        buffers->data[i] = (uint8_t*)malloc(chMaxSamples * bytesPerSample * outSRate * 360);
        buffers->data_len[i] = 0;
    }
    
    std::thread readingThread(readingRoutine, buffers, audioCirBuffer, destinationPlanarFrame);

    while(mngr->isRunning()) {
        if ((codedFrame = q->getFront()) == NULL) {
            usleep(100);
            continue;
        }

        if(!audioDecoder->decodeFrame(codedFrame, aFrame)) {
            std::cout << "Error decoding frames\n";
        }

        q->removeFrame();

       // fillBuffer(buffers, aFrame, aFrame->getChannels(), aFrame->getLength());
        
        while(!audioCirBuffer->pushBack(aFrame->getPlanarDataBuf(), aFrame->getSamples())) {
            printf("Push back failed! Trying again\n");
            usleep(100);
        }
    }

    should_stop = true;
    //saveBuffer(buffers);
    readingThread.join();
    
    return 0;
}