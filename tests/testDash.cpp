#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/modules/audioMixer/AudioMixer.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#include "../src/modules/videoMixer/VideoMixer.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/modules/dasher/Dasher.hh"
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

#define RETRIES 60

#define SEG_DURATION 4 //sec
#define DASH_FOLDER "/tmp/dash"
#define BASE_NAME "test"

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    Controller::getInstance()->pipelineManager()->stop();
    run = false;
    exit(0);
}

Dasher* setupDasher(int dasherId)
{
    Dasher* dasher = NULL;

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    dasher = Dasher::createNew(std::string(DASH_FOLDER), std::string(BASE_NAME), SEG_DURATION);

    if(!dasher) {
        utils::errorMsg("Error configure dasher: exist testTranscoder");
        exit(1);
    }

    pipe->addFilter(dasherId, dasher);

    return dasher;
}

void addAudioPath(unsigned port, Dasher* dasher, int dasherId, int receiverID)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int decId = rand();
    int encId = rand();
    int dstReader = rand();
    std::vector<int> ids({decId, encId});

    AudioDecoderLibav *decoder;
    AudioEncoderLibav *encoder;

    Path *path;

    //NOTE: Adding decoder to pipeManager and handle worker
    decoder = new AudioDecoderLibav();
    pipe->addFilter(decId, decoder);

    //NOTE: Adding encoder to pipeManager and handle worker
    encoder = new AudioEncoderLibav();
    if (!encoder->configure(OUT_A_CODEC, A_CHANNELS, OUT_A_FREQ, OUT_A_BITRATE)) {
        utils::errorMsg("Error configuring audio encoder. Check provided parameters");
        return;
    }

    pipe->addFilter(encId, encoder);

    path = pipe->createPath(receiverID, dasherId, port, dstReader, ids);
    pipe->addPath(port, path);
    if (!pipe->connectPath(path)){
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return;
    }

    if (dasher != NULL && !dasher->addSegmenter(dstReader)) {
        utils::errorMsg("Error adding segmenter");
    }

    if (dasher != NULL && !dasher->setDashSegmenterBitrate(dstReader, OUT_A_BITRATE)) {
        utils::errorMsg("Error setting bitrate to segmenter");
    }

    utils::infoMsg("Audio path created from port " + std::to_string(port));
}

void addVideoPath(unsigned port, Dasher* dasher, int dasherId, int receiverID, 
                    unsigned width = 0, unsigned height = 0)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int decId = rand();
    int resId = 2000;
    int encId = 1000;
    int dstReader1 = 3000;

    std::vector<int> ids;
    ids = {decId, resId, encId};

    std::string sessionId;
    std::string sdp;

    VideoDecoderLibav *decoder;
    VideoResampler *resampler;
    VideoEncoderX264 *encoder;

    Path *path;

    //NOTE: Adding decoder to pipeManager and handle worker
    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);

    //NOTE: Adding resampler to pipeManager and handle worker
    resampler = new VideoResampler();
    pipe->addFilter(resId, resampler);
    resampler->configure(1280, 720, 0, YUV420P);

    //NOTE: Adding encoder to pipeManager and handle worker
    encoder = new VideoEncoderX264();
    pipe->addFilter(encId, encoder);

    //bitrate, fps, gop, lookahead, threads, annexB, preset
    encoder->configure(4000, 25, 25, 25, 4, true, "superfast");

    path = pipe->createPath(receiverID, dasherId, port, dstReader1, ids);
    
    pipe->addPath(port, path);
    if (!pipe->connectPath(path)){
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return;
    }

    if (!dasher->addSegmenter(dstReader1)) {
        utils::errorMsg("Error adding segmenter");
    }
    if (!dasher->setDashSegmenterBitrate(dstReader1, 4000*1000)) {
        utils::errorMsg("Error setting bitrate to segmenter");
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

bool addRTSPsession(std::string rtspUri, Dasher* dasher, int dasherId,
                    SourceManager *receiver, int receiverID)
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
            addVideoPath(subsession->clientPortNum(), dasher, dasherId, receiverID);
        } else if (medium.compare("audio") == 0){
            addAudioPath(subsession->clientPortNum(), dasher, dasherId, receiverID);
        }
    }

    return true;
}

bool publishRTSPSession(std::vector<int> readers, SinkManager *transmitter)
{
    std::string sessionId;

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

    return true;
}

int main(int argc, char* argv[])
{
    int vPort = 0;
    int aPort = 0;
    std::string rtspUri;
    Dasher* dasher = NULL;
    int dasherId = 4000;
    int receiverID = rand();

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
        } else if (strcmp(argv[i],"-r")==0) {
            rtspUri = argv[i+1];
            utils::infoMsg("input RTSP URI: " + rtspUri);
            utils::infoMsg("Ignoring any audio or video input port, just RTSP inputs");
        }
    }

    if (vPort == 0 && aPort == 0 && rtspUri.length() == 0){
        utils::errorMsg("invalid parameters");
        return 1;
    }

    receiver = new SourceManager();
    pipe = Controller::getInstance()->pipelineManager();

    dasher = setupDasher(dasherId);
    pipe->addFilter(receiverID, receiver);

    signal(SIGINT, signalHandler);

    if (vPort != 0 && rtspUri.length() == 0){
        addVideoSDPSession(vPort, receiver);
        addVideoPath(vPort, dasher, dasherId, receiverID);
    }

    if (aPort != 0 && rtspUri.length() == 0){
        addAudioSDPSession(aPort, receiver);
        addAudioPath(aPort, dasher, dasherId, receiverID);
    }

    if (rtspUri.length() > 0){
        if (!addRTSPsession(rtspUri, dasher, dasherId, receiver, receiverID)){
            utils::errorMsg("Couldn't start rtsp client session!");
            return 1;
        }
    }

    while(run) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
