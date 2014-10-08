#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/AudioFrame.hh"
#include "../src/Controller.hh"
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
#define V_TIME_STMP_FREQ 90000
#define FRAME_RATE 25

#define A_MEDIUM "audio"
#define A_PAYLOAD 97
#define A_CODEC "OPUS"
#define A_BANDWITH 128
#define A_TIME_STMP_FREQ 48000
#define A_CHANNELS 2

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->stop();
    run = false;
    
    utils::infoMsg("Workers Stopped");
}

int createOutputPathAndSessions() 
{
    int mixId = rand();
    int encId = rand();
    int mixWorkerId = rand();
    int encWorkerId = rand();
    int pathId = rand();
    std::vector<int> ids({encId});
    std::vector<int> readers;

    std::string sessionId;

    AudioMixer *mixer;
    AudioEncoderLibav *encoder;

    ConstantFramerateMaster *mixWorker;
    ConstantFramerateMaster *encWorker;

    Path* path;

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SinkManager *transmitter = pipe->getTransmitter();

    //NOTE: Adding mixer to pipeManager and handle worker
    mixer = new AudioMixer();
    pipe->addFilter(mixId, mixer);
    mixWorker = new ConstantFramerateMaster();
    mixWorker->addProcessor(mixId, mixer);
    mixer->setWorkerId(mixWorkerId);
    pipe->addWorker(mixWorkerId, mixWorker);
    mixWorker->setFps(1/0.020);

    //NOTE: Adding encoder to pipeManager and handle worker
    encoder = new AudioEncoderLibav();
    pipe->addFilter(encId, encoder);
    encWorker = new ConstantFramerateMaster();
    encWorker->addProcessor(encId, encoder);
    encoder->setWorkerId(encWorkerId);
    pipe->addWorker(encWorkerId, encWorker);
    encWorker->setFps(1/0.024);
   
    //NOTE: add filter to path
    path = pipe->createPath(mixId, pipe->getTransmitterID(), -1, -1, ids);
    pipe->addPath(pathId, path);       
    pipe->connectPath(path);

    readers.push_back(path->getDstReaderID());

    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (! transmitter->addSession(sessionId, readers)){
        return 1;
    }
    transmitter->publishSession(sessionId);
        
    pipe->startWorkers();

    return mixId;
}

void addAudioSource(unsigned port, int mixerId, std::string codec = A_CODEC, 
                    unsigned channels = A_CHANNELS, unsigned freq = A_TIME_STMP_FREQ)
{
    int aDecId = rand();
    int decId = rand();
    std::vector<int> ids({decId});
    std::string sessionId;
    std::string sdp;
    
    AudioDecoderLibav *decoder;
    
    BestEffortMaster* aDec;
    
    Session *session;
    Path *path;
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
       
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is an audio stream");    
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, codec, 
                                            A_BANDWITH, freq, port, channels);
    utils::infoMsg(sdp);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    if (!receiver->addSession(session)){
        utils::errorMsg("Could not add audio session");
        return;
    }
    if (!session->initiateSession()){
        utils::errorMsg("Could not initiate audio session");
        return;
    }
    
    //NOTE: Adding decoder to pipeManager and handle worker
    decoder = new AudioDecoderLibav();
    pipe->addFilter(decId, decoder);
    aDec = new BestEffortMaster();
    aDec->addProcessor(decId, decoder);
    decoder->setWorkerId(aDecId);
    pipe->addWorker(aDecId, aDec);
    
    //NOTE: add filter to path
    path = pipe->createPath(pipe->getReceiverID(), mixerId, port, port, ids);
    pipe->addPath(port, path);       
    pipe->connectPath(path);
        
    pipe->startWorkers();
}

int main(int argc, char* argv[]) 
{   
    std::vector<int> readers;
    std::vector<unsigned> ports;
    
    int mixerId;

    utils::setLogLevel(INFO);
    
    for (int i = 1; i < argc; i++) {
        ports.push_back(std::stoi(argv[i]));     
        utils::infoMsg("Audio input port added: " + std::to_string(std::stoi(argv[i])));
    }

    if (ports.empty()) {
        utils::errorMsg("Usage: testAudio <port1> <port2> ... <portN>");
        return 1;
    }
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->start();

    signal(SIGINT, signalHandler);

    mixerId = createOutputPathAndSessions(); 

    for (auto it : ports) {
        addAudioSource(it, mixerId);
    }
       
    while (run) {
        sleep(1);
    }
 
    return 0;
}

