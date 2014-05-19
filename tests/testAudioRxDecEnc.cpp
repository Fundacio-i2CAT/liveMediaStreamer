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

#include "../src/AudioFrame.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
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
    unsigned char* data;
    int data_len;
};

void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    SourceManager *mngr = SourceManager::getInstance();
    mngr->closeManager();
    
    std::cout << "Manager closed\n";
}

void fillBuffer(struct buffer *b, Frame *pFrame) {
    memcpy(b->data + b->data_len, pFrame->getDataBuf(), pFrame->getLength());
    b->data_len += pFrame->getLength(); 
}

void saveBuffer(struct buffer *b) 
{
    FILE *audioChannel = NULL;
    char filename[32];

    sprintf(filename, "coded.opus");

    audioChannel = fopen(filename, "wb");

    if (b->data != NULL) {
        fwrite(b->data, b->data_len, 1, audioChannel);
    }

    fclose(audioChannel);
}

void readingRoutine(struct buffer* b, AudioCircularBuffer* cb, AudioEncoderLibav* enc)
{
    Frame *fr;

    InterleavedAudioFrame* codedFrame = InterleavedAudioFrame::createNew (
                                                    enc->getChannels(), 
                                                    enc->getSampleRate(), 
                                                    enc->getSamplesPerFrame(), 
                                                    enc->getCodec(), 
                                                    S16
                                                  );

    while(!should_stop) {
        fr = cb->getFront();

        if(!fr) {
            usleep(100);
            continue;
        }

        if(!enc->encodeFrame(fr, codedFrame)) {
            std::cerr << "Error encoding frame" << std::endl;
        }

        fillBuffer(b, codedFrame);
        printf("Filled buffer! Frame size: %d\n", codedFrame->getLength());
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
    AudioEncoderLibav* audioEncoder;
    std::map<unsigned short, FrameQueue*> inputs;
    FrameQueue* q;
    AudioCircularBuffer* audioCirBuffer;
    Frame* aFrame;
    PlanarAudioFrame* destinationPlanarFrame;
    Frame *codedFrame;
    struct buffer *buffers;

    ACodecType inCType = OPUS;
    ACodecType outCType = PCM;
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
    if(!audioDecoder->configure(inCType, inSFmt, inCh, inSRate, outSFmt, outCh, outSRate)) {
        std::cerr << "Error configuring decoder" << std::endl;
    }

    audioEncoder = new AudioEncoderLibav();
    if(!audioEncoder->configure(PCMU, outSFmt, outCh, outSRate)) {
        std::cerr << "Error configuring encoder" << std::endl;
    }

    audioCirBuffer = AudioCircularBuffer::createNew(outCh, outSRate, chMaxSamples, outSFmt);

    inputs = mngr->getInputs();
    q = inputs[A_CLIENT_PORT];
    buffers = new struct buffer;
    buffers->data = new unsigned char[chMaxSamples * bytesPerSample * outSRate * 360]();
    buffers->data_len = 0;
    
    std::thread readingThread(readingRoutine, buffers, audioCirBuffer, audioEncoder);

    while(mngr->isRunning()) {
        if ((codedFrame = q->getFront()) == NULL) {
            usleep(100);
            continue;
        }

        aFrame = audioCirBuffer->getRear();

        if(!audioDecoder->doProcessFrame(codedFrame, aFrame)) {
            std::cout << "Error decoding frames\n";
        }

        q->removeFrame();
        audioCirBuffer->addFrame();
    }

    should_stop = true;
    readingThread.join();
    
    return 0;
}