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

#ifndef _VIDEO_DECODER_LIBAV_HH
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#endif

#ifndef _FRAME_QUEUE_HH
#include "../src/FrameQueue.hh"
#endif

#include <fstream>
#include <iostream>
#include <csignal>

#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define PAYLOAD 96
#define V_CODEC "H264"
#define BANDWITH 5000
#define V_CLIENT_PORT 6004
#define V_TIME_STMP_FREQ 90000

#define A_CODEC "AC3"
#define A_CLIENT_PORT 6006
#define A_MEDIUM "audio"
#define A_TIME_STMP_FREQ 44100


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
    Frame* codedFrame;
    Frame* rawFrame = new Frame();
    VideoDecoderLibav* decoder = new VideoDecoderLibav();
    std::ofstream rawFrames;
    
    //condif decoder
    if (! decoder->configDecoder(H264, RGB24)){
        return 1;
    }
    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = handlers::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i]);
        mngr->addSession(sessionId, session);
    }
    
//     sessionId = handlers::randomIdGenerator(ID_LENGTH);
//     
//     sdp = handlers::makeSessionSDP("testSession", "this is a test");
//     
//     sdp += handlers::makeSubsessionSDP(V_MEDIUM, PROTOCOL, PAYLOAD, V_CODEC, 
//                                        BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT);
//     sdp += handlers::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD, A_CODEC, 
//                                        BANDWITH, A_TIME_STMP_FREQ, A_CLIENT_PORT);
//     
//     session = Session::createNew(*(mngr->envir()), sdp);
//     
//     mngr->addSession(sessionId, session);
    
    mngr->runManager();
    
    mngr->initiateAll();
    
    //Let some time to initiate reciver sessions
    sleep(2);
    
    queue = mngr->getInputs().begin()->second;
    
    while(mngr->isRunning()){
        if ((codedFrame = queue->getFront()) == NULL){
            continue;
        } else {
            decoder->decodeFrame(codedFrame, rawFrame);
            queue->removeFrame();
            if (! rawFrames.is_open()){
                rawFrames.open("frames.yuv", std::ios::out | std::ios::app | std::ios::binary);
            } 
            if (rawFrame->getLength() > 0) {
                rawFrames.write(reinterpret_cast<const char*>(rawFrame->getDataBuf()), rawFrame->getLength());
            }
        }
        usleep(1000);
    }
    
    rawFrames.close();
    
    return 0;
}

