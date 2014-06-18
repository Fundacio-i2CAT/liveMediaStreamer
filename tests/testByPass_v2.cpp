#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#include <string>

#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/Utils.hh"

#include <csignal>
#include <iostream>
#include <vector>

#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define PAYLOAD 96
#define V_CODEC "H264"
#define BANDWITH 5000
#define V_CLIENT_PORT 6004
#define V_TIME_STMP_FREQ 90000


void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    SourceManager *receiver = SourceManager::getInstance();
    SinkManager *transmitter = SinkManager::getInstance();
    receiver->closeManager();
    transmitter->closeManager();
    
    std::cout << "Managers closed\n";
}

void connect(char const* medium, unsigned short port){}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    std::vector<int> readers;
    Session* session;

    SourceManager *receiver = SourceManager::getInstance();
    SinkManager *transmitter = SinkManager::getInstance();
    
    //This will connect every input directly to the transmitter
    receiver->setCallback((void(*)(char const*, unsigned short))&connect);
    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = utils::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(receiver->envir()), argv[0], argv[i], sessionId);
        receiver->addSession(session);
    }
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    
    sdp = SourceManager::makeSessionSDP(sessionId, "this is a test");
    
    sdp += SourceManager::makeSubsessionSDP(V_MEDIUM, PROTOCOL, PAYLOAD, V_CODEC, 
                                       BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    
    receiver->addSession(session);

    session->initiateSession();
       
    receiver->runManager();
    transmitter->runManager();
    
    receiver->connectManyToMany(transmitter, V_CLIENT_PORT, V_CLIENT_PORT);
    
    sleep(1);
    
    readers.push_back(V_CLIENT_PORT);
    
    if (! transmitter->addSession(sessionId, readers)){
        return 1;
    }
    
    transmitter->publishSession(sessionId);

    while(receiver->isRunning() && transmitter->isRunning()) {
        sleep(1);
    }

    return 0;
}