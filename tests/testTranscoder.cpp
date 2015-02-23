#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/sharedMemory/SharedMemory.hh"
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

#define RETRIES 60

#define SEG_DURATION 4 //sec
#define DASH_FOLDER "/home/palau/nginx_root/dashLMS"
#define BASE_NAME "test"
#define MPD_LOCATION "http://localhost/dashLMS/test.mpd"

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->stop();
    run = false;

    utils::infoMsg("Workers Stopped");
}

Dasher* setupDasher(int dasherId)
{
    Dasher* dasher;

    int workerId = rand();
    Worker* worker = NULL;
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    dasher = new Dasher();
    if(!dasher->configure(DASH_FOLDER, BASE_NAME, SEG_DURATION, MPD_LOCATION)) {
        exit(1);
    }

    pipe->addFilter(dasherId, dasher);
    worker = new Worker();
    worker->addProcessor(dasherId, dasher);
    dasher->setWorkerId(workerId);
    pipe->addWorker(workerId, worker);

    return dasher;
}

void addAudioPath(unsigned port, Dasher* dasher, int dasherId)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int aDecId = rand();
    int aEncId = rand();
    int decId = rand();
    int encId = rand();
    int dstReader = rand();
    std::vector<int> ids({decId, encId});

    AudioDecoderLibav *decoder;
    AudioEncoderLibav *encoder;

    Worker* aDec;
    Worker* aEnc;

    Path *path;

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
    if (dasher == NULL){
        path = pipe->createPath(pipe->getReceiverID(), pipe->getTransmitterID(), port, -1, ids);
    } else {
        path = pipe->createPath(pipe->getReceiverID(), dasherId, port, dstReader, ids);
    }
    pipe->addPath(port, path);
    pipe->connectPath(path);

    if (dasher != NULL && !dasher->addSegmenter(dstReader)) {
        utils::errorMsg("Error adding segmenter");
    }

    pipe->startWorkers();

    utils::infoMsg("Audio path created from port " + std::to_string(port));
}

void addVideoPath(unsigned port, Dasher* dasher, int dasherId, size_t sharingMemoryKey = 0,  unsigned width = 0, unsigned height = 0)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int wResId = rand();
    int wResId2 = rand();
    int wEncId = rand();
    int wEncId2 = rand();
    int wDecId = rand();
    int decId = rand();
    int resId = rand();
    int resId2 = rand();
    int encId = rand();
    int encId2 = rand();
    int dstReader1 = rand();
    int dstReader2 = rand();
    int slavePathId = rand();
    int shmId = rand();
    int wShmId = rand();
    SharedMemory *shm;
    Worker* wShm;
    int shmEncId = rand();
    int wEncShmId = rand();
    SharedMemory *shmEnc;
    Worker* wEncShm;

    std::vector<int> ids({decId, resId, encId});
    std::vector<int> slaveIds({encId2});

    if(sharingMemoryKey>0){
        ids.clear();
        ids.push_back(decId);
        ids.push_back(shmId);
        ids.push_back(resId);
        ids.push_back(encId);
        ids.push_back(shmEncId);
    }

    std::string sessionId;
    std::string sdp;

    VideoResampler *resampler;
    VideoResampler *resampler2;
    VideoEncoderX264 *encoder;
    VideoEncoderX264 *encoder2;
    VideoDecoderLibav *decoder;

    Worker* wDec;
    Worker* wRes;
    Worker* wRes2;
    Worker* wEnc;
    Worker* wEnc2;

    Path *path, *slavePath;

    //NOTE: Adding decoder to pipeManager and handle worker
    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);
    wDec = new Worker();
    wDec->addProcessor(decId, decoder);
    decoder->setWorkerId(wDecId);
    pipe->addWorker(wDecId, wDec);

    //NOTE: Adding sharedMemory to pipeManager and handle worker
    if(sharingMemoryKey > 0){
        shm = SharedMemory::createNew(sharingMemoryKey, RAW);
        if(!shm){
            utils::errorMsg("Could not initiate sharedMemory filter");
            exit(1);
        }
        pipe->addFilter(shmId, shm);
        wShm = new Worker();
        wShm->addProcessor(shmId, shm);
        shm->setWorkerId(wShmId);
        pipe->addWorker(wShmId, wShm);
    }

    //NOTE: Adding resampler to pipeManager and handle worker
    resampler = new VideoResampler();
    pipe->addFilter(resId, resampler);
    wRes = new Worker();
    wRes->addProcessor(resId, resampler);
    resampler->setWorkerId(wResId);
    resampler->configure(width, height, 0, YUV420P);
    pipe->addWorker(wResId, wRes);

    //NOTE: Adding encoder to pipeManager and handle worker
    encoder = new VideoEncoderX264();
    pipe->addFilter(encId, encoder);
    wEnc = new Worker();
    wEnc->addProcessor(encId, encoder);
    encoder->setWorkerId(wEncId);
    pipe->addWorker(wEncId, wEnc);

    if(sharingMemoryKey > 0){
        shmEnc = SharedMemory::createNew(sharingMemoryKey + 1, H264);
        if(!shmEnc){
            utils::errorMsg("Could not initiate sharedMemory filter");
            exit(1);
        }
        pipe->addFilter(shmEncId, shmEnc);
        wEncShm = new Worker();
        wEncShm->addProcessor(shmId, shmEnc);
        shmEnc->setWorkerId(wEncShmId);
        pipe->addWorker(wEncShmId, wEncShm);
    }

    if (dasher != NULL){
        path = pipe->createPath(pipe->getReceiverID(), dasherId, port, dstReader1, ids);
    } else {
        path = pipe->createPath(pipe->getReceiverID(), pipe->getTransmitterID(), port, -1, ids);
    }
    pipe->addPath(port, path);
    pipe->connectPath(path);

    if (dasher != NULL){
        //NOTE: Adding resampler to pipeManager and handle worker
        resampler2 = new VideoResampler(SLAVE);
        pipe->addFilter(resId2, resampler2);
        wRes2 = new Worker();
        wRes2->addProcessor(resId2, resampler2);
        resampler2->setWorkerId(wResId2);
        resampler2->configure(640, 480, 0, YUV420P);
        pipe->addWorker(wResId2, wRes2);
        ((BaseFilter*)resampler)->addSlave(resId2, resampler2);

        //NOTE: Adding encoder to pipeManager and handle worker
        encoder2 = new VideoEncoderX264(SLAVE, VIDEO_DEFAULT_FRAMERATE, false);
        pipe->addFilter(encId2, encoder2);
        wEnc2 = new Worker();
        wEnc2->addProcessor(encId2, encoder2);
        encoder2->setWorkerId(wEncId2);
        pipe->addWorker(wEncId2, wEnc2);
        ((BaseFilter*)encoder)->addSlave(wEncId2, encoder2);

        //NOTE: add filter to path
        slavePath = pipe->createPath(resId2, dasherId, -1, dstReader2, slaveIds);
        pipe->addPath(slavePathId, slavePath);
        if (!slavePath) {
            std::cout << "NO slave path...." << std::endl;
        }
        pipe->connectPath(slavePath);

        utils::infoMsg("Master reader: " + std::to_string(dstReader1));
        utils::infoMsg("Slave reader: " + std::to_string(dstReader2));

        if (!dasher->addSegmenter(dstReader1)) {
            utils::errorMsg("Error adding segmenter");
        }

        if (!dasher->addSegmenter(dstReader2)) {
            utils::errorMsg("Error adding segmenter");
        }
    }

    pipe->startWorkers();

    utils::infoMsg("Video path created from port " + std::to_string(port));
}

bool addVideoSDPSession(unsigned port, std::string codec = V_CODEC)
{
    Session *session;
    std::string sessionId;
    std::string sdp;

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
        return false;
    }
    if (!session->initiateSession()){
        utils::errorMsg("Could not initiate video session");
        return false;
    }

    return true;
}

bool addAudioSDPSession(unsigned port, std::string codec = A_CODEC,
                        unsigned channels = A_CHANNELS, unsigned freq = A_TIME_STMP_FREQ)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();

    Session *session;
    std::string sessionId;
    std::string sdp;

    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is an audio stream");
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, codec,
                                            A_BANDWITH, freq, port, channels);
    utils::infoMsg(sdp);

    session = Session::createNew(*(receiver->envir()), sdp, sessionId, receiver);
    if (!receiver->addSession(session)){
        utils::errorMsg("Could not add audio session");
        return false;
    }
    if (!session->initiateSession()){
        utils::errorMsg("Could not initiate audio session");
        return false;
    }

    return true;
}

bool addRTSPsession(std::string rtspUri, Dasher* dasher, int dasherId)
{
    Session* session;
    std::string sessionId = utils::randomIdGenerator(ID_LENGTH);
    std::string medium;
    unsigned retries = 0;

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();

    session = Session::createNewByURL(*(receiver->envir()), "testTranscoder", rtspUri, sessionId, receiver);
    if (!receiver->addSession(session)){
        utils::errorMsg("Could not add rtsp session");
        return false;
    }

    if (!session->initiateSession()){
        utils::errorMsg("Could not initiate video session");
        return false;
    }

    while (session->getScs()->session == NULL && retries <= RETRIES){
        sleep(1);
        retries++;
    }

    MediaSubsessionIterator iter(*(session->getScs()->session));
    MediaSubsession* subsession;

    while(iter.next() == NULL && retries <= RETRIES){
        sleep(1);
        retries++;
    }

    if (retries > RETRIES){
        delete receiver;
        return false;
    }

    utils::infoMsg("RTSP client session created!");

    iter.reset();

    while((subsession = iter.next()) != NULL){
        medium = subsession->mediumName();

        if (medium.compare("video") == 0){
            addVideoPath(subsession->clientPortNum(), dasher, dasherId);
        } else if (medium.compare("audio") == 0){
            addAudioPath(subsession->clientPortNum(), dasher, dasherId);
        }
    }

    return true;
}

bool publishRTSPSession(std::vector<int> readers)
{
    std::string sessionId;
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SinkManager *transmitter = pipe->getTransmitter();

    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (!transmitter->addRTSPConnection(readers, 1, STD_RTP, sessionId)){
        return false;
    }

    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (!transmitter->addRTSPConnection(readers, 2, MPEGTS, sessionId)){
        return false;
    }

    return true;
}

void addConnections(std::vector<int> readers, std::string ip, unsigned port)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SinkManager *transmitter = pipe->getTransmitter();
    if (transmitter->addRTPConnection(readers, rand(), ip, port, MPEGTS)) {
        utils::infoMsg("added connection for " + ip + ":" + std::to_string(port));
    }
}

int main(int argc, char* argv[])
{
    int vPort = 0;
    int aPort = 0;
    int port = 0;
    std::string ip;
    std::string rtspUri;
    size_t sharingMemoryKey = 0;
	bool dash = false;
    Dasher* dasher = NULL;
    int dasherId = rand();
    std::vector<int> readers;

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
        } else if (strcmp(argv[i],"-r")==0) {
            rtspUri = argv[i+1];
            utils::infoMsg("input RTSP URI: " + rtspUri);
            utils::infoMsg("Ignoring any audio or video input port, just RTSP inputs");
        } else if (strcmp(argv[i],"-dash")==0) {
            dash = true;
            utils::infoMsg("Output will be DASH, ignoring any -P, -d or -ts flag");
        } else if (strcmp(argv[i],"-s")==0) {
            sharingMemoryKey = std::stoi(argv[i+1]);
            utils::infoMsg("sharing memory key set to: "+ std::to_string(sharingMemoryKey));
        }
    }

    if (vPort == 0 && aPort == 0 && rtspUri.length() == 0){
        utils::errorMsg("Invalid parameters. Usage: testranscoder -v <video_input_port> -a <audio_input_port> -d <dst_ip> -P <dst_port> -r <rtsp_uri> -dash");
        return 1;
    }

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    if (! pipe->start()){
        utils::errorMsg("Couldn't start pipe");
        return 1;
    }

    signal(SIGINT, signalHandler);

    if (dash){
        dasher = setupDasher(dasherId);
    }

    if (vPort != 0 && rtspUri.length() == 0){
        addVideoSDPSession(vPort);
        addVideoPath(vPort, dasher, dasherId, sharingMemoryKey);
    }

    if (aPort != 0 && rtspUri.length() == 0){
        addAudioSDPSession(aPort);
        addAudioPath(aPort, dasher, dasherId);
    }

    if (rtspUri.length() > 0){
        if (!addRTSPsession(rtspUri, dasher, dasherId)){
            utils::errorMsg("Couldn't start rtsp client session!");
            return 1;
        }
    }

    for (auto it : pipe->getPaths()) {
        readers.push_back(it.second->getDstReaderID());
    }

    if (!dash) {
        publishRTSPSession(readers);
    }

    if (!dash && port != 0 && !ip.empty()){
        addConnections(readers, ip, port);
    }

    while (run) {
        sleep(1);
    }

    return 0;
}
