#include "../src/network/Handlers.hh"
#include "../src/network/SourceManager.hh"
#include "../src/AudioFrame.hh"
#include "../src/Controller.hh"
#include "../src/Callbacks.hh"

#include <iostream>
#include <csignal>
#include <vector>
#include <liveMedia/liveMedia.hh>
#include <string>

#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define V_PAYLOAD 96
#define V_CODEC "H264"
#define V_BANDWITH 1200
#define V_CLIENT_PORT 6004
#define V_TIME_STMP_FREQ 90000

#define A_MEDIUM "audio"
#define A_PAYLOAD 14
#define A_CODEC "MP3"
#define A_BANDWITH 705
#define A_CLIENT_PORT 6006
#define A_TIME_STMP_FREQ 44100
#define A_CHANNELS 2


void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    Controller *ctrl = Controller::getInstance();
    SourceManager *receiver = ctrl->pipelineManager()->getReceiver();
    SinkManager *transmitter = ctrl->pipelineManager()->getTransmitter();
    receiver->closeManager();
    transmitter->closeManager();
    
    std::cout << "Managers closed\n";
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    std::vector<int> readers;
    Session* session;

    Controller *ctrl = Controller::getInstance();
    SourceManager *receiver = ctrl->pipelineManager()->getReceiver();
    SinkManager *transmitter = ctrl->pipelineManager()->getTransmitter();
    
    //This will connect every input directly to the transmitter
    receiver->setCallback(callbacks::connectToTransmitter);

    Frame *codedFrame;
    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = handlers::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(receiver->envir()), argv[0], argv[i], sessionId);
        receiver->addSession(session);
        session->initiateSession();
    }
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    
    sdp = handlers::makeSessionSDP(sessionId, "this is a test");
    
    sdp += handlers::makeSubsessionSDP(V_MEDIUM, PROTOCOL, V_PAYLOAD, V_CODEC, 
                                       V_BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT);
    
    sdp += handlers::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, A_CODEC, 
                                       A_BANDWITH, A_TIME_STMP_FREQ, A_CLIENT_PORT, A_CHANNELS);
    
    std::cout << sdp << "\n\n";
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    
    receiver->addSession(session);

    session->initiateSession();
       
    receiver->runManager();
    transmitter->runManager();
    
    sleep(2);
    
    for (auto it : ctrl->pipelineManager()->getPaths()){
        readers.push_back(it.second->getDstReaderID());    
    }
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    if (! transmitter->addSession(sessionId, readers)){
        return 1;
    }
    transmitter->publishSession(sessionId);

    while(receiver->isRunning() && transmitter->isRunning()) {
        sleep(1);
    }

    return 0;
}
