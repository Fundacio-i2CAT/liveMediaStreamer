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
#define V_TIME_STMP_FREQ 90000
#define FRAME_RATE 20

#define A_MEDIUM "audio"
#define A_PAYLOAD 97
#define A_CODEC "PCMU"
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

void addAudioSource(unsigned port, std::string codec = A_CODEC, 
                    unsigned channels = A_CHANNELS, unsigned freq = A_TIME_STMP_FREQ)
{
    int aDecId = rand();
    int aEncId = rand();
    int id;
    std::string sessionId;
    std::string sdp;
    AudioEncoderLibav *aEncoder;
    Session *session;
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
    
    BestEffortMaster* aDec = new BestEffortMaster();
    BestEffortMaster* aEnc = new BestEffortMaster();
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is an audio stream");    
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, codec, 
                                            A_BANDWITH, freq, port, channels);
    utils::infoMsg(sdp);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    receiver->addSession(session);
    session->initiateSession();
    
    //Let liveMedia start all the filters
    usleep(100000);
    
    id = pipe->searchFilterIDByType(AUDIO_DECODER);
    aDec->addProcessor(id, pipe->getFilter(id));
    pipe->getFilter(id)->setWorkerId(aDecId);
    
    id = pipe->searchFilterIDByType(AUDIO_ENCODER);
    aEncoder = dynamic_cast<AudioEncoderLibav*> (pipe->getFilter(id));
    aEnc->addProcessor(id, aEncoder);
    aEncoder->setWorkerId(aEncId);
    
    pipe->addWorker(aDecId, aDec);
    pipe->addWorker(aEncId, aEnc);
    
    pipe->startWorkers();
}

void addVideoSource(unsigned port, std::string codec = V_CODEC, 
                    unsigned width = 0, unsigned height = 0, unsigned fps = FRAME_RATE)
{
    int wResId = rand();
    int wEncId = rand();
    int wDecId = rand();
    int id;
    std::string sessionId;
    std::string sdp;
    VideoResampler *resampler;
    VideoEncoderX264 *encoder;
    Session *session;
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
    
    BestEffortMaster* wRes = new BestEffortMaster();
    ConstantFramerateMaster* wEnc = new ConstantFramerateMaster();
    BestEffortMaster* wDec = new BestEffortMaster();
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is a video stream");    
    sdp += SourceManager::makeSubsessionSDP(V_MEDIUM, PROTOCOL, V_PAYLOAD, codec, 
                                            V_BANDWITH, V_TIME_STMP_FREQ, port);
    utils::infoMsg(sdp);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    receiver->addSession(session);
    session->initiateSession();
    
    //Let liveMedia start all the filters
    usleep(100000);
    
    id = pipe->searchFilterIDByType(VIDEO_RESAMPLER);
    resampler = dynamic_cast<VideoResampler*> (pipe->getFilter(id));
    wRes->addProcessor(id, resampler);
    resampler->setWorkerId(wResId);
    
    id = pipe->searchFilterIDByType(VIDEO_ENCODER);
    encoder = dynamic_cast<VideoEncoderX264*> (pipe->getFilter(id));
    wEnc->addProcessor(id, encoder);
    encoder->setWorkerId(wEncId);
    
    id = pipe->searchFilterIDByType(VIDEO_DECODER);
    wDec->addProcessor(id, pipe->getFilter(id));
    pipe->getFilter(id)->setWorkerId(wDecId);
    
    resampler->configure(width, height, 0, YUV420P);
    wEnc->setFps(fps*1000.0/1005);
    
    pipe->addWorker(wResId, wRes);
    pipe->addWorker(wEncId, wEnc);
    pipe->addWorker(wDecId, wDec);
    
    pipe->startWorkers();
}

void addConnections(std::vector<int> readers, std::string ip, unsigned port)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SinkManager *transmitter = pipe->getTransmitter();
    for(auto reader : readers){
        transmitter->addConnection(reader, ip, port);
        utils::infoMsg("added connection for " + ip + ":" + std::to_string(port));
        port+=2;
    }
}

int main(int argc, char* argv[]) 
{   
    std::vector<int> readers;
    
    unsigned vPort = 0;
    unsigned aPort = 0;
    unsigned port = 0;
    std::string ip;
    std::string sessionId;

    utils::setLogLevel(INFO);
    
    for (int i = 1; i < argc; i++) {     
        if (strcmp(argv[i],"-v")==0) {
            vPort = std::stoi(argv[i+1]);
            printf("video input port: %d",vPort);
        } else if (strcmp(argv[i],"-a")==0) {
            aPort = std::stoi(argv[i+1]);
            printf("audio input port: %d",aPort);
        } else if (strcmp(argv[i],"-d")==0) {
            ip = argv[i + 1];
            printf("\ndestination IP:%s",ip.c_str());
        } else if (strcmp(argv[i],"-P")==0) {
            port = std::stoi(argv[i+1]);
            printf("destination port: %d",port);
        }
    }
    
    if (vPort == 0 && aPort == 0){
        utils::errorMsg("invalid parameters");
        return 1;
    }

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->start();
    SourceManager *receiver = pipe->getReceiver();
    SinkManager *transmitter = pipe->getTransmitter();

    receiver->setCallback(callbacks::connectTranscoderToTransmitter);
    signal(SIGINT, signalHandler); 
    pipe->startWorkers();
    
    if (vPort != 0){
        addVideoSource(vPort);
    }
    
    if (aPort != 0){
        addAudioSource(aPort);
    }
       
    for (auto it : pipe->getPaths()){
        readers.push_back(it.second->getDstReaderID());    
    }
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (! transmitter->addSession(sessionId, readers)){
        return 1;
    }
    transmitter->publishSession(sessionId);
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (! transmitter->addSession(sessionId, readers)){
        return 1;
    }
    transmitter->publishSession(sessionId);
    
    if (port != 0 && !ip.empty()){
        addConnections(readers, ip, port);
    }
    
    while (run) {
        sleep(5);
        pipe->stop();
        break;
    }
 
    return 0;
}

