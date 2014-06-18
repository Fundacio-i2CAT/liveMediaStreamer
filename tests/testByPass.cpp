#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/AudioFrame.hh"
#include "../src/Controller.hh"
#include "../src/Callbacks.hh"
#include "../src/Utils.hh"

#include <iostream>
#include <csignal>
#include <vector>
#include <liveMedia/liveMedia.hh>
#include <string>

#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define PAYLOAD 96
#define V_CODEC "VP8"
#define BANDWITH 5000
#define V_CLIENT_PORT 6004
#define V_TIME_STMP_FREQ 90000


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
        sessionId = utils::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(receiver->envir()), argv[0], argv[i], sessionId);
        receiver->addSession(session);
        session->initiateSession();
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
    
    sleep(2);
    
    for (auto it : ctrl->pipelineManager()->getPaths()){
        readers.push_back(it.second->getDstReaderID());
    
        sessionId = utils::randomIdGenerator(ID_LENGTH);
        if (! transmitter->addSession(sessionId, readers)){
            return 1;
        }
        readers.pop_back();
        transmitter->publishSession(sessionId);
    }

    while(receiver->isRunning() && transmitter->isRunning()) {
        sleep(1);
    }

    return 0;
}
