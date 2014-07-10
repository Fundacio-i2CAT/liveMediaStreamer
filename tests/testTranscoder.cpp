#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/AudioFrame.hh"
#include "../src/Controller.hh"
#include "../src/Callbacks.hh"
#include "../src/Utils.hh"

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

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->stopWorkers();
    
    utils::infoMsg("Workers Stopped");
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    std::vector<int> readers;
    Session* session;
    int id, count = 0;
    VideoResampler *resampler;
    VideoEncoderX264 *encoder;

    utils::setLogLevel(INFO);
    
    Controller *ctrl = Controller::getInstance();
    PipelineManager *pipe = ctrl->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
    SinkManager *transmitter = pipe->getTransmitter();
    
    //This will connect every input directly to the transmitter
    receiver->setCallback(callbacks::connectTranscoderToTransmitter);
     
    pipe->startWorkers();
    
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
    
    //sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, A_CODEC, 
                                       //A_BANDWITH, A_TIME_STMP_FREQ, A_CLIENT_PORT, A_CHANNELS);
    
    utils::infoMsg(sdp);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    
    receiver->addSession(session);

    session->initiateSession();
    
    sleep(1);
       
    for (auto it : pipe->getPaths()){
        readers.push_back(it.second->getDstReaderID());    
    }
    
    id = pipe->searchFilterIDByType(VIDEO_RESAMPLER);
    resampler = dynamic_cast<VideoResampler*> (pipe->getFilter(id));
    
    id = pipe->searchFilterIDByType(VIDEO_ENCODER);
    encoder = dynamic_cast<VideoEncoderX264*> (pipe->getFilter(id));
    
    resampler->configure(0, 0, 0, YUV420P);
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (! transmitter->addSession(sessionId, readers)){
        return 1;
    }
    
    transmitter->publishSession(sessionId);
    transmitter->addConnection(readers.front(), "127.0.0.1", 3030);
    
    while(pipe->getWorker(pipe->getReceiverID())->isRunning() || 
        pipe->getWorker(pipe->getTransmitterID())->isRunning()) {
        sleep(1);
        if (count == 10){
            resampler->configure(1280, 534, 2, YUV420P);
            encoder->configure(12, 12);
            utils::infoMsg("Half frame rate");
        } 
        if (count == 20){
            resampler->configure(640, 534, 0, YUV420P);
            encoder->configure(24, 24);
            utils::infoMsg("Regular frame rate");
            count = 0;
        }
        count++;
    }

    return 0;
}
