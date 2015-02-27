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

#define OUT_A_CODEC AAC

#define SEG_DURATION 4 //sec
#define DASH_FOLDER "/home/palau/nginx_root/dashLMS"
#define BASE_NAME "test"
#define MPD_LOCATION "http://localhost/dashLMS/test.mpd"


bool run = true;
int dasherId = rand();
Dasher* dasher = NULL;

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
    Worker* worker = NULL;
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    
    dasher = Dasher::createNew(DASH_FOLDER, BASE_NAME, SEG_DURATION, MPD_LOCATION);

    if(!dasher) {
        exit(1);
    }

    pipe->addFilter(dasherId, dasher);
    worker = new Worker();
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
    int dstReader = rand();

    std::vector<int> ids({decId, encId});
    std::string sessionId;
    std::string sdp;
    
    AudioDecoderLibav *decoder;
    AudioEncoderLibav *encoder;
    
    Worker* aDec;
    Worker* aEnc;
    
    Session *session;
    Path *path;
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
       
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is an audio stream");    
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, codec, 
                                            A_BANDWITH, freq, port, channels);
    utils::infoMsg(sdp);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId, receiver);
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
    aDec = new Worker();
    aDec->addProcessor(decId, decoder);
    decoder->setWorkerId(aDecId);
    pipe->addWorker(aDecId, aDec);
    
    //NOTE: Adding encoder to pipeManager and handle worker
    encoder = new AudioEncoderLibav();
    if (!encoder->setup(OUT_A_CODEC, A_CHANNELS, A_TIME_STMP_FREQ)) {
        utils::errorMsg("Error configuring audio encoder. Check provided parameters");
        return;
    }
    pipe->addFilter(encId, encoder);
    aEnc = new Worker();
    aEnc->addProcessor(encId, encoder);
    encoder->setWorkerId(aEncId);
    pipe->addWorker(aEncId, aEnc);

    //NOTE: add filter to path
    path = pipe->createPath(pipe->getReceiverID(), dasherId, port, dstReader, ids);
    pipe->addPath(port, path);       
    pipe->connectPath(path);

    if (!dasher->addSegmenter(dstReader)) {
        utils::errorMsg("Error adding segmenter");
    }

    pipe->startWorkers();
}

void addVideoSource(unsigned port, unsigned fps = FRAME_RATE, std::string codec = V_CODEC, 
                    unsigned width = 0, unsigned height = 0)
{
    int wRes1Id = rand();
    int wRes2Id = rand();
    int wEnc1Id = rand();
    int wEnc2Id = rand();
    int wDecId = rand();
    int decId = rand();
    int res1Id = rand();
    int res2Id = rand();
    int enc1Id = rand();
    int enc2Id = rand();
    int dstReader1 = rand();
    int dstReader2 = rand();

    std::vector<int> masterIds({decId, res1Id, enc1Id});
    std::vector<int> slaveIds({enc2Id});
    std::string sessionId;
    std::string sdp;

    VideoResampler *resampler1;
    VideoResampler *resampler2;
    VideoEncoderX264 *encoder1;
    VideoEncoderX264 *encoder2;
    VideoDecoderLibav *decoder;
    
    Worker* wDec;
    Worker* wRes1;
    Worker* wRes2;
    Worker* wEnc1;
    Worker* wEnc2;
    
    Session *session;
    Path *masterPath;
    Path *slavePath;
    int slavePathId = rand();
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is a video stream");    
    sdp += SourceManager::makeSubsessionSDP(V_MEDIUM, PROTOCOL, V_PAYLOAD, codec, 
                                            V_BANDWITH, V_TIME_STMP_FREQ, port);
    utils::infoMsg(sdp);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId, receiver);
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
    wDec = new Worker();
    wDec->addProcessor(decId, decoder);
    decoder->setWorkerId(wDecId);
    pipe->addWorker(wDecId, wDec);
    
    //NOTE: Adding resampler to pipeManager and handle worker
    resampler1 = new VideoResampler();
    pipe->addFilter(res1Id, resampler1);
    wRes1 = new Worker();
    wRes1->addProcessor(res1Id, resampler1);
    resampler1->setWorkerId(wRes1Id);
    resampler1->configure(width, height, 0, YUV420P);
    pipe->addWorker(wRes1Id, wRes1);

    //NOTE: Adding resampler to pipeManager and handle worker
    resampler2 = new VideoResampler(SLAVE);
    pipe->addFilter(res2Id, resampler2);
    wRes2 = new Worker();
    wRes2->addProcessor(res2Id, resampler2);
    resampler2->setWorkerId(wRes2Id);
    resampler2->configure(640, 480, 0, YUV420P);
    pipe->addWorker(wRes2Id, wRes2);
    ((BaseFilter*)resampler1)->addSlave(res2Id, resampler2);
    
    //NOTE: Adding encoder to pipeManager and handle worker
    encoder1 = new VideoEncoderX264();
    pipe->addFilter(enc1Id, encoder1);
    wEnc1 = new Worker();
    wEnc1->addProcessor(enc1Id, encoder1);
    encoder1->setWorkerId(wEnc1Id);
    pipe->addWorker(wEnc1Id, wEnc1);

    //NOTE: Adding encoder to pipeManager and handle worker
    encoder2 = new VideoEncoderX264(SLAVE, VIDEO_DEFAULT_FRAMERATE, false);
    pipe->addFilter(enc2Id, encoder2);
    wEnc2 = new Worker();
    wEnc2->addProcessor(enc2Id, encoder2);
    encoder2->setWorkerId(wEnc2Id);
    pipe->addWorker(wEnc2Id, wEnc2);
    ((BaseFilter*)encoder1)->addSlave(res2Id, encoder2);

    //NOTE: add filter to path
    masterPath = pipe->createPath(pipe->getReceiverID(), dasherId, port, dstReader1, masterIds);
    pipe->addPath(port, masterPath);       
    pipe->connectPath(masterPath);

    //NOTE: add filter to path
    slavePath = pipe->createPath(res2Id, dasherId, -1, dstReader2, slaveIds);
    pipe->addPath(slavePathId, slavePath);       
    pipe->connectPath(slavePath);

    if (!dasher->addSegmenter(dstReader1)) {
        utils::errorMsg("Error adding segmenter");
    }

    if (!dasher->addSegmenter(dstReader2)) {
        utils::errorMsg("Error adding segmenter");
    }
    
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