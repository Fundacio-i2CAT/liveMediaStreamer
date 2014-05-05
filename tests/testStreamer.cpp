#include "../src/network/SourceManager.hh"
#include <liveMedia.hh>
#include <string>
#include "../src/network/Handlers.hh"
#include <iostream>
#include <csignal>
#include "../src/network/H264QueueServerMediaSubsession.hh"
#include "../src/network/SinkManager.hh"


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


void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    SourceManager *mngr = SourceManager::getInstance();
    SinkManager* sMngr = SinkManager::getInstance();
    mngr->closeManager();
    sMngr->closeManager();
    
    std::cout << "Managers closed\n";
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    Session* session;
    ServerMediaSession* sSession;
    SourceManager* mngr = SourceManager::getInstance();
    SinkManager* sMngr = SinkManager::getInstance();
    FrameQueue* queue;
    
    signal(SIGINT, signalHandler); 
    
    sMngr->runManager();
    mngr->runManager();
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = handlers::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i]);
        mngr->addSession(sessionId, session);
    }
  
    mngr->initiateAll();
    
    sSession = ServerMediaSession::createNew(*(sMngr->envir()), "serverTestSession", 
                                             "serverTestSession", 
                                             "this is a server test");
    sleep(1);
    
    queue = mngr->getInputs().front();
    
    sSession->addSubsession(H264QueueServerMediaSubsession::createNew(*(sMngr->envir()), 
                                                                      queue, False));
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    sMngr->addSession(sessionId, sSession);
    sMngr->publishSession(sessionId);
    
    while(mngr->isRunning() || sMngr->isRunning()){
        sleep(1);
    }
    
    return 0;
}
