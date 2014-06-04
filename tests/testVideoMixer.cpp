#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#include <string>

#include "../src/network/Handlers.hh"
#include "../src/network/SourceManager.hh"
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#include "../src/modules/videoMixer/VideoMixer.hh"
#include "../src/FrameQueue.hh"
#include "../src/VideoFrame.hh"



#include <fstream>
#include <iostream>
#include <csignal>

#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define PAYLOAD 96
#define V_CODEC "H264"
#define BANDWITH 5000
#define V_CLIENT_PORT1 6004
#define V_CLIENT_PORT2 7004
#define V_TIME_STMP_FREQ 90000



void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    SourceManager *mngr = SourceManager::getInstance();
    mngr->closeManager();
    
    std::cout << "Manager closed\n";
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    Session* session; 
    SourceManager *mngr = SourceManager::getInstance();
    FrameQueue* queue;
    Frame* rawFrame = InterleavedVideoFrame::createNew(RAW, 1280, 720, RGB24);
    VideoDecoderLibav* decoder1 = new VideoDecoderLibav();
    VideoDecoderLibav* decoder2 = new VideoDecoderLibav();
    Worker *vDecoderWorker1;
    Worker *vDecoderWorker2;
    Worker *vMixerWorker;
    std::ofstream rawFrames;

    int layoutWidth = 1280;
    int layoutHeight = 720;
    int mixerChannels = 2;
    VideoMixer* mixer = new VideoMixer(mixerChannels, layoutWidth, layoutHeight);

    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = handlers::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i], sessionId);
        mngr->addSession(session);
    }
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    sdp = handlers::makeSessionSDP("testSession1", "this is a test");
    sdp += handlers::makeSubsessionSDP(V_MEDIUM, PROTOCOL, PAYLOAD, V_CODEC, 
                                       BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT1);
    session = Session::createNew(*(mngr->envir()), sdp, sessionId);
    mngr->addSession(session);
    session->initiateSession();

    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    sdp = handlers::makeSessionSDP("testSession2", "this is a test");
    sdp += handlers::makeSubsessionSDP(V_MEDIUM, PROTOCOL, PAYLOAD, V_CODEC, 
                                       BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT2);
    session = Session::createNew(*(mngr->envir()), sdp, sessionId);
    mngr->addSession(session);
    session->initiateSession();
    
    mngr->runManager();
       
    //Let some time to initiate reciver sessions
    sleep(2);

    int id1 = mngr->getWriterID(V_CLIENT_PORT1);
    int id2 = mngr->getWriterID(V_CLIENT_PORT2);

    if(!mngr->connect(id1, decoder1, decoder1->getAvailableReaders().front())) {
        std::cerr << "Error connecting sourceManager with videoDecoder1" << std::endl;
    }

    if(!mngr->connect(id2, decoder2, decoder2->getAvailableReaders().front())) {
        std::cerr << "Error connecting sourceManager with videoDecoder2" << std::endl;
    }

    int mixerChannel1 = mixer->getAvailableReaders().front();

    if(!decoder1->connect(decoder1->getAvailableWriters().front(), mixer, mixerChannel1)) {
        std::cerr << "Error connecting decoder1 with mixer" << std::endl;
    }

    int mixerChannel2 = mixer->getAvailableReaders().front();

    if(!decoder2->connect(decoder2->getAvailableWriters().front(), mixer, mixerChannel2)) {
        std::cerr << "Error connecting decoder2 with mixer" << std::endl;
    }

    Reader *reader = new Reader();
    mixer->connect(mixer->getAvailableWriters().front(), reader);

    if(!mixer->setPositionSize(mixerChannel1, 0.5, 0.5, 0, 0, 0)) {
        std::cerr << "Error configuring mixer channel1" << std::endl;
    }

    decoder1->configure(layoutWidth*0.5, layoutHeight*0.5, RGB24);

    if(!mixer->setPositionSize(mixerChannel2, 0.5, 0.5, 0.5, 0, 1)) {
        std::cerr << "Error configuring mixer channel2" << std::endl;
    }

    decoder2->configure(layoutWidth*0.5, layoutHeight*0.5, RGB24);

    vDecoderWorker1 = new Worker(decoder1);
    vDecoderWorker2 = new Worker(decoder2);
    vMixerWorker = new Worker(mixer, 25);

    vDecoderWorker1->start(); 
    vDecoderWorker2->start(); 
    vMixerWorker->start(); 
    
    while(mngr->isRunning()) {
        rawFrame = reader->getFrame();

        if (!rawFrame) {
            usleep(500);
            continue;
        }

        if (!rawFrames.is_open()){
            rawFrames.open("frames.rgb", std::ios::out | std::ios::app | std::ios::binary);
        } 
        if (rawFrame->getLength() > 0) {
            rawFrames.write(reinterpret_cast<const char*>(rawFrame->getDataBuf()), rawFrame->getLength());
           // printf("Filled buffer! Frame size: %d\n", rawFrame->getLength());
        }
        
        reader->removeFrame();
    }
    
    rawFrames.close();
    
    return 0;
}
