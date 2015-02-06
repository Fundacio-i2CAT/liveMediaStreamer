#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/dasher/Dasher.hh"
#include "../src/AudioFrame.hh"
#include "../src/Controller.hh"
#include "../src/Utils.hh"

#include <csignal>
#include <vector>
#include <liveMedia.hh>
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

#define SEGMENT_DURATION 8000000 //us

bool run = true;
int dasherId = rand();

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->stop();
    run = false;
    
    utils::infoMsg("Workers Stopped");
}

void setupDasher() 
{
    int workerId = rand();
    Master* worker = NULL;
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    
    Dasher* dasher = new Dasher(SEGMENT_DURATION);
    pipe->addFilter(dasherId, dasher);
    worker = new Master();
    worker->addProcessor(dasherId, dasher);
    dasher->setWorkerId(workerId);
    pipe->addWorker(workerId, worker);
}

void addAudioSource(unsigned port, std::string codec = A_CODEC, 
                    unsigned channels = A_CHANNELS, unsigned freq = A_TIME_STMP_FREQ)
{
    int aDecId = rand();
    int aEncId = rand();
    int decId = rand();
    int encId = rand();
    std::vector<int> ids({decId, encId});
    std::string sessionId;
    std::string sdp;
    
    AudioDecoderLibav *decoder;
    AudioEncoderLibav *encoder;
    
    Master* aDec;
    Master* aEnc;
    
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
    aDec = new Master();
    aDec->addProcessor(decId, decoder);
    decoder->setWorkerId(aDecId);
    pipe->addWorker(aDecId, aDec);
    
    //NOTE: Adding encoder to pipeManager and handle worker
    encoder = new AudioEncoderLibav();
    pipe->addFilter(encId, encoder);
    aEnc = new Master();
    aEnc->addProcessor(encId, encoder);
    encoder->setWorkerId(aEncId);
    pipe->addWorker(aEncId, aEnc);

    //NOTE: add filter to path
    path = pipe->createPath(pipe->getReceiverID(), dasherId, port, -1, ids);
    pipe->addPath(port, path);       
    pipe->connectPath(path);
        
    pipe->startWorkers();
}

void addVideoSource(unsigned port, unsigned fps = FRAME_RATE, std::string codec = V_CODEC, 
                    unsigned width = 0, unsigned height = 0)
{
    int wResId = rand();
    int wEncId = rand();
    int wDecId = rand();
    int decId = rand();
    int resId = rand();
    int encId = rand();
    std::vector<int> ids({decId, resId, encId});
    std::string sessionId;
    std::string sdp;
    
    VideoResampler *resampler;
    VideoEncoderX264 *encoder;
    VideoDecoderLibav *decoder;
    
    Master* wDec;
    Master* wRes;
    Master* wEnc;
    
    Session *session;
    Path *path;
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is a video stream");    
    sdp += SourceManager::makeSubsessionSDP(V_MEDIUM, PROTOCOL, V_PAYLOAD, codec, 
                                            V_BANDWITH, V_TIME_STMP_FREQ, port);
    utils::infoMsg(sdp);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    if (!receiver->addSession(session)){
        utils::errorMsg("Could not add video session");
        return;
    }
    if (!session->initiateSession()){
        utils::errorMsg("Could not initiate video session");
        return;
    }
       
    //NOTE: Adding decoder to pipeManager and handle worker
    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);
    wDec = new Master();
    wDec->addProcessor(decId, decoder);
    decoder->setWorkerId(wDecId);
    pipe->addWorker(wDecId, wDec);
    
    //NOTE: Adding resampler to pipeManager and handle worker
    resampler = new VideoResampler();
    pipe->addFilter(resId, resampler);
    wRes = new Master();
    wRes->addProcessor(resId, resampler);
    resampler->setWorkerId(wResId);
    resampler->configure(width, height, 0, YUV420P);
    pipe->addWorker(wResId, wRes);
    
    //NOTE: Adding encoder to pipeManager and handle worker
    encoder = new VideoEncoderX264();
    pipe->addFilter(encId, encoder);
    wEnc = new Master();
    wEnc->addProcessor(encId, encoder);
    encoder->setWorkerId(wEncId);
    pipe->addWorker(wEncId, wEnc);
    
    //NOTE: add filter to path
    path = pipe->createPath(pipe->getReceiverID(), dasherId, port, -1, ids);
    pipe->addPath(port, path);       
    pipe->connectPath(path);
    
    pipe->startWorkers();
}

int main(int argc, char* argv[]) 
{   
    std::vector<int> readers;

    int vPort = 0;
    int aPort = 0;
    std::string ip;
    std::string sessionId;
    std::string rtspUri;

    utils::setLogLevel(INFO);
    
    for (int i = 1; i < argc; i++) {     
        if (strcmp(argv[i],"-v")==0) {
            vPort = std::stoi(argv[i+1]);
            utils::infoMsg("video input port: " + std::to_string(vPort));
        } else if (strcmp(argv[i],"-a")==0) {
            aPort = std::stoi(argv[i+1]);
            utils::infoMsg("audio input port: " + std::to_string(aPort));
        }
    }
    
    if (vPort == 0 && aPort == 0){
        utils::errorMsg("invalid parameters");
        return 1;
    }

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->start();
    setupDasher();

    signal(SIGINT, signalHandler); 
    
    if (vPort != 0){
        addVideoSource(vPort);
    }
    
    if (aPort != 0){
        addAudioSource(aPort);
    }
    
    while (run) {
        sleep(1);
    }
 
    return 0;
}