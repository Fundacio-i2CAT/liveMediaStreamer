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
#define A_CODEC "MPEG4_GENERIC"
#define A_BANDWITH 32
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
	std::vector<int> ids;
    std::string sessionId;
    std::string sdp;
    
    Session *session;
    Path *path;
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
       
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is an audio stream");    
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, "MPEG4-GENERIC", 
                                            A_BANDWITH, freq, port, channels);
	sdp += "a=fmtp:97 profile-level-id=1;mode=aac-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=EB8A0800\n";
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
    
    //NOTE: add filter to path
    path = pipe->createPath(pipe->getReceiverID(), pipe->getTransmitterID(), port, -1, ids);
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
    encoder = new VideoEncoderX264(true);
    encoder->configure(DEFAULT_GOP, DEFAULT_BITRATE, DEFAULT_ENCODER_THREADS, true);
    pipe->addFilter(encId, encoder);
    wEnc = new Master();
    wEnc->addProcessor(encId, encoder);
    encoder->setWorkerId(wEncId);
    pipe->addWorker(wEncId, wEnc);
    
    //NOTE: add filter to path
    path = pipe->createPath(pipe->getReceiverID(), pipe->getTransmitterID(), port, -1, ids);
    pipe->addPath(port, path);       
    pipe->connectPath(path);
    
    pipe->startWorkers();
}

void addConnections(std::vector<int> readers, std::string ip, unsigned port)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SinkManager *transmitter = pipe->getTransmitter();
    for(auto reader : readers){
        if (transmitter->addConnection(reader, rand(), ip, port)) {
            utils::infoMsg("added connection for " + ip + ":" + std::to_string(port));
            port+=2;
        }
    }
}

void addDashConnections(std::vector<int> readers, std::string fileName, bool reInit, uint32_t segmentTime, uint32_t fps = FRAME_RATE)
{
	std::string quality = "192";
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SinkManager *transmitter = pipe->getTransmitter();
    for(auto reader : readers){
        if (transmitter->addDashConnection(reader, rand(), fileName, quality, reInit, segmentTime, 0, fps)) {
            utils::infoMsg("added connection for " + fileName);
        }
    }
}

int main(int argc, char* argv[]) 
{   
    std::vector<int> readers;
    
    unsigned vPort = 0;
    unsigned aPort = 0;
    unsigned port = 0;
    unsigned fps = FRAME_RATE;
	unsigned segmentTime = 2;
	bool reInit = false;
    std::string ip;
    std::string sessionId;
    std::string rtspUri;
	std::string fileName;
	std::string fileNameAudio;

    utils::setLogLevel(INFO);
    
    for (int i = 1; i < argc; i++) {     
        if (strcmp(argv[i],"-v")==0) {
            vPort = std::stoi(argv[i+1]);
            utils::infoMsg("video input port: " + std::to_string(vPort));
        } else if (strcmp(argv[i],"-a")==0) {
            aPort = std::stoi(argv[i+1]);
            utils::infoMsg("audio input port: " + std::to_string(aPort));
        } else if (strcmp(argv[i],"-d")==0) {
            ip = argv[i + 1];
            utils::infoMsg("destination IP: " + ip);
        } else if (strcmp(argv[i],"-P")==0) {
            port = std::stoi(argv[i+1]);
            utils::infoMsg("destination port: " + std::to_string(port));
        } else if (strcmp(argv[i],"-f")==0) {
            fps = std::stoi(argv[i+1]);
            utils::infoMsg("output frame rate: " + std::to_string(fps));
        } else if (strcmp(argv[i],"-D")==0) {
            fileName = argv[i+1];
            utils::infoMsg("fileName " + fileName);
        } else if (strcmp(argv[i],"-A")==0) {
            fileNameAudio = argv[i+1];
            utils::infoMsg("fileName " + fileNameAudio);
        }
    }
    
    if (vPort == 0 && aPort == 0){
        utils::errorMsg("invalid parameters");
        return 1;
    }

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->start();
    SinkManager *transmitter = pipe->getTransmitter();

    signal(SIGINT, signalHandler); 
    
    if (vPort != 0){
        addVideoSource(vPort, fps);
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

    if (!fileName.empty()){
        addDashConnections(readers, fileName, reInit, segmentTime, fps);
    }

    if (!fileNameAudio.empty()){
        addDashConnections(readers, fileNameAudio, reInit, segmentTime);
    }   
 
    while (run) {
        sleep(1);
    }
 
    return 0;
}

