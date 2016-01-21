#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/modules/audioMixer/AudioMixer.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/videoEncoder/VideoEncoderX265.hh"
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#include "../src/modules/videoMixer/VideoMixer.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/receiver/SourceManager.hh"
#include "../src/modules/transmitter/SinkManager.hh"
#include "../src/AudioFrame.hh"
#include "../src/Controller.hh"
#include "../src/Utils.hh"
#include "../src/modules/sharedMemory/SharedMemory.hh"

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
// #define A_CODEC "MPEG4-GENERIC"
#define A_CODEC "OPUS"
#define A_BANDWITH 128
#define A_TIME_STMP_FREQ 48000
#define A_CHANNELS 2

#define OUT_A_CODEC AAC
#define OUT_A_FREQ 48000
#define OUT_A_BITRATE 192000

#define RETRIES 10

#define SEG_DURATION 4 //sec
#define BASE_NAME "test"

#define BYPASS_VIDEO_PATH 3113
#define BYPASS_AUDIO_PATH 4224

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
}

void addAudioPath(unsigned port, int receiverID, int transmitterID)
{
    PipelineManager *pipe = PipelineManager::getInstance();

    int decId = 101;
    int encId = 5000;
    std::vector<int> ids({decId, encId});

    AudioDecoderLibav *decoder;
    AudioEncoderLibav *encoder;

    decoder = new AudioDecoderLibav();
    pipe->addFilter(decId, decoder);

    encoder = new AudioEncoderLibav();
    if (!encoder->configure(OUT_A_CODEC, A_CHANNELS, OUT_A_FREQ, OUT_A_BITRATE)) {
        utils::errorMsg("Error configuring audio encoder. Check provided parameters");
        return;
    }

    pipe->addFilter(encId, encoder);
    
    if (!pipe->createPath(BYPASS_AUDIO_PATH, receiverID, transmitterID, port, BYPASS_AUDIO_PATH, std::vector<int>({}))) {
        utils::errorMsg("Error creating audio path");
        return;
    }

    if (!pipe->connectPath(BYPASS_AUDIO_PATH)) {
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return;
    }
    
    if (!pipe->createPath(port, receiverID, transmitterID, port, -1, ids)) {
        utils::errorMsg("Error creating audio path");
        return;
    }
    
    if (!pipe->connectPath(port)){
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return;
    }

    utils::infoMsg("Audio path created from port " + std::to_string(port));
}

void addVideoPath(unsigned port, int receiverID, int transmitterID)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int decId = 500;
    int resId = 2000;
    int encId = 1000;

    std::vector<int> ids;

    ids = {decId, resId, encId};

    std::string sessionId;
    std::string sdp;

    VideoResampler *resampler;
    VideoEncoderX264 *encoder;
    VideoDecoderLibav *decoder;

    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);

    resampler = new VideoResampler();
    pipe->addFilter(resId, resampler);
    resampler->configure(1280, 720, 0, YUV420P);

    encoder = new VideoEncoderX264();
    pipe->addFilter(encId, encoder);

    //bitrate, fps, gop, lookahead, threads, annexB, preset
    encoder->configure(4000, 0, 25, 15, 4, true, "superfast");
    
    if (!pipe->createPath(BYPASS_VIDEO_PATH, receiverID, transmitterID, port, BYPASS_VIDEO_PATH, std::vector<int>({}))) {
        utils::errorMsg("Error creating video path");
        return;
    }

    if (!pipe->connectPath(BYPASS_VIDEO_PATH)) {
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return;
    }
    
    if (!pipe->createPath(port, receiverID, transmitterID, port, -1, ids)) {
        utils::errorMsg("Error creating video path");
        return;
    }   

    if (!pipe->connectPath(port)) {
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return;
    }

    utils::infoMsg("Video path created from port " + std::to_string(port));
}

bool addVideoSDPSession(unsigned port, SourceManager *receiver, std::string codec = V_CODEC)
{
    Session *session;
    std::string sessionId;
    std::string sdp;

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

bool addAudioSDPSession(unsigned port, SourceManager *receiver, std::string codec = A_CODEC,
                        unsigned channels = A_CHANNELS, unsigned freq = A_TIME_STMP_FREQ)
{
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

bool addRTSPsession(std::string rtspUri, SourceManager *receiver, int receiverID, int transmitterID)
{
    Session* session;
    std::string sessionId = utils::randomIdGenerator(ID_LENGTH);
    std::string medium;
    unsigned retries = 0;

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
    
    while(true){
        if (retries > RETRIES){
            delete receiver;
            return false;
        }
        
        sleep(1);
        retries++;
        
        if ((subsession = iter.next()) == NULL){
            iter.reset();
            continue;
        }
        
        if (subsession->clientPortNum() > 0){
            iter.reset();
            break;
        }
    }

    utils::infoMsg("RTSP client session created!");
    
    while((subsession = iter.next()) != NULL){
        medium = subsession->mediumName();

        if (medium.compare("video") == 0){
            addVideoPath(subsession->clientPortNum(), receiverID, transmitterID);
        } else if (medium.compare("audio") == 0){
            addAudioPath(subsession->clientPortNum(), receiverID, transmitterID);
        }
    }

    return true;
}

bool publishRTSPSession(std::vector<int> readers, SinkManager *transmitter)
{
    std::string sessionId;
    std::vector<int> byPassReaders;

    sessionId = "plainrtp";
    utils::infoMsg("Adding plain RTP session...");
    if (!transmitter->addRTSPConnection(readers, 1, STD_RTP, sessionId)){
        return false;
    }

    sessionId = "mpegts";
    utils::infoMsg("Adding plain MPEGTS session...");
    if (!transmitter->addRTSPConnection(readers, 2, MPEGTS, sessionId)){
        return false;
    }

    
    if (transmitter->isRConnected(BYPASS_AUDIO_PATH)){
        byPassReaders.push_back(BYPASS_AUDIO_PATH);
    }
    
    if (transmitter->isRConnected(BYPASS_VIDEO_PATH)){
        byPassReaders.push_back(BYPASS_VIDEO_PATH);
    }
    
    sessionId = "bypass";
    utils::infoMsg("Adding bypass session...");
    if (!transmitter->addRTSPConnection(byPassReaders, 3, STD_RTP, sessionId)){
        return false;
    }
    
    return true;
}

int main(int argc, char* argv[])
{
    int vPort = 0;
    int aPort = 0;
    int port = 0;
    int cPort = 7777;
    std::string ip;
    std::string rtspUri;
    std::vector<int> readers;

    int transmitterID = 1024;
    int receiverID = 1023;

    SinkManager* transmitter = NULL;
    SourceManager* receiver = NULL;
    PipelineManager *pipe;

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
        } else if (strcmp(argv[i],"-c")==0) {
            cPort = std::stoi(argv[i+1]);
            utils::infoMsg("control port: " + std::to_string(cPort));
        }
    }

    if (vPort == 0 && aPort == 0 && rtspUri.length() == 0){
        utils::errorMsg("invalid parameters");
        return 1;
    }

    receiver = new SourceManager();
    pipe = Controller::getInstance()->pipelineManager();
    
    transmitter = SinkManager::createNew();
    if (!transmitter){
        utils::errorMsg("RTSPServer constructor failed");
        return 1;
    }

    pipe->addFilter(transmitterID, transmitter);        
    pipe->addFilter(receiverID, receiver);

    signal(SIGINT, signalHandler);

    if (vPort != 0 && rtspUri.length() == 0){
        addVideoSDPSession(vPort, receiver);
        addVideoPath(vPort, receiverID, transmitterID);
    }

    if (aPort != 0 && rtspUri.length() == 0){
        addAudioSDPSession(aPort, receiver);
        addAudioPath(aPort, receiverID, transmitterID);
    }

    if (rtspUri.length() > 0){
        if (!addRTSPsession(rtspUri, receiver, receiverID, transmitterID)){
            utils::errorMsg("Couldn't start rtsp client session!");
            return 1;
        }
    }

    for (auto it : pipe->getPaths()) {
        if (it.second->getDstReaderID() != BYPASS_VIDEO_PATH && it.second->getDstReaderID() != BYPASS_AUDIO_PATH){
            readers.push_back(it.second->getDstReaderID());
        }
    }
    
    if (readers.empty()){
        utils::errorMsg("No readers provided!");
        return 1;
    }

    if (!publishRTSPSession(readers, transmitter)){
        utils::errorMsg("Failed adding RTSP sessions!");
        return 1;
    }

    if (port != 0 && !ip.empty()){
        if (transmitter->addRTPConnection(readers, rand(), ip, port, STD_RTP)) {
            utils::infoMsg("added connection for " + ip + ":" + std::to_string(port));
        }
    }

    Controller* ctrl = Controller::getInstance();

    if (!ctrl->createSocket(cPort)) {
        exit(1);
    }

    while (run) {
        if (!ctrl->listenSocket()) {
            continue;
        }

        if (!ctrl->readAndParse()) {
            utils::errorMsg("Controller failed to read and parse the incoming event data");
            continue;
        }

        ctrl->processRequest();
    }

    pipe->destroyInstance();
    utils::infoMsg("Pipe deleted");
    ctrl->destroyInstance();
    utils::infoMsg("Controller deleted");
    
    return 0;
}
