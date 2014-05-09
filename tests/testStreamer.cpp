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

#ifndef _SINK_MANAGER_HH
#include "../src/network/SinkManager.hh"
#endif

#ifndef _H264_QUEUE_SERVER_MEDIA_SESSION_HH
#include "../src/network/H264QueueServerMediaSubsession.hh"
#endif

#ifndef _FRAME_QUEUE_HH
#include "../src/FrameQueue.hh"
#endif

#include <iostream>
#include <csignal>


#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define PAYLOAD 96
#define V_CODEC "H264"
#define BANDWITH 5000
#define V_CLIENT_PORT 6004
#define V_TIME_STMP_FREQ 90000

#define A_CODEC "AAC"
#define A_CLIENT_PORT 6006
#define A_MEDIUM "audio"
#define A_TIME_STMP_FREQ 48000


void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    SourceManager *mngr = SourceManager::getInstance();
    SinkManager *sMngr = SinkManager::getInstance();
    mngr->closeManager();
    sMngr->closeManager();
    
    std::cout << "Managers closed\n";
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    Session* session;
    FrameQueue *vQueue, *aQueue;
    SourceManager *mngr = SourceManager::getInstance();
    SinkManager *sMngr = SinkManager::getInstance();
    
    mngr->runManager();
    sMngr->runManager();
    
    signal(SIGINT, signalHandler); 
    
    //receiver sessions
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = handlers::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i]);
        mngr->addSession(sessionId, session);
    }
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
     
    sdp = handlers::makeSessionSDP("testSession", "this is a test");
     
    sdp += handlers::makeSubsessionSDP(V_MEDIUM, PROTOCOL, PAYLOAD, V_CODEC, 
                                        BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT);
    sdp += handlers::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD, A_CODEC, 
                                       BANDWITH, A_TIME_STMP_FREQ, A_CLIENT_PORT);
    
    std::cout << sdp << std::endl;
    
    session = Session::createNew(*(mngr->envir()), sdp);
    
    mngr->addSession(sessionId, session);
     
    mngr->initiateAll();
    
    //transitter sessions
    
    //Let some time to initiate reciver sessions
    sleep(2);
    
    if (mngr->getInputs().empty()){
        mngr->closeManager();
        sMngr->closeManager();
        return 1;
    }
    
    vQueue = mngr->getInputs()[6004];
    aQueue = mngr->getInputs()[6006];
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    
    ServerMediaSession* servSession
        = ServerMediaSession::createNew(*(sMngr->envir()), "testServerSession", 
                                        "testServerSession", "this is a test");
    servSession->addSubsession(H264QueueServerMediaSubsession::createNew(
        *(sMngr->envir()), vQueue, True));
    
    sMngr->addSession(sessionId, servSession);
    sMngr->publishSession(sessionId);
    
    while(mngr->isRunning() || sMngr->isRunning()){
        sleep(1);
    }
    
    return 0;
}


