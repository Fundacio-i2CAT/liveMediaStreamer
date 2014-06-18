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
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
    SinkManager *transmitter = pipe->getTransmitter();
    receiver->closeManager();
    pipe->getWorker(pipe->getTransmitterID())->stop();
    
    std::cout << "Managers closed\n";
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    std::vector<int> readers;
    Session* session;

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
    SinkManager *transmitter = pipe->getTransmitter();
    
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
    
    sdp += SourceManager::makeSubsessionSDP(V_MEDIUM, PROTOCOL, V_PAYLOAD, V_CODEC, 
                                       V_BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT);
    
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, A_CODEC, 
                                       A_BANDWITH, A_TIME_STMP_FREQ, A_CLIENT_PORT, A_CHANNELS);
    
    std::cout << sdp << "\n\n";
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    
    receiver->addSession(session);

    session->initiateSession();
       
    receiver->runManager();
    
    pipe->getWorker(pipe->getTransmitterID())->start();
    
    sleep(2);
    
    for (auto it : pipe->getPaths()){
        readers.push_back(it.second->getDstReaderID());    
    }
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (! transmitter->addSession(sessionId, readers)){
        return 1;
    }
    transmitter->publishSession(sessionId);

    while(receiver->isRunning() && 
        pipe->getWorker(pipe->getTransmitterID())->isRunning()) {
        sleep(1);
    }

    return 0;
}
