#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/modules/audioMixer/AudioMixer.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/videoEncoder/VideoEncoderX265.hh"
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

#define MAX_VIDEO_QUALITIES 3
#define DEFAULT_OUTPUT_VIDEO_QUALITIES 3
#define DEFAULT_FIRST_VIDEO_QUALITY 2000

#define SEG_DURATION 4 //sec
#define DASH_FOLDER "/tmp/dash"
#define BASE_NAME "test"

#define BYPASS_VIDEO_PATH 3113
#define BYPASS_AUDIO_PATH 4224

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
}

Dasher* setupDasher(int dasherId, std::string dash_folder, int segDuration)
{
    Dasher* dasher = NULL;

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    dasher = new Dasher();

    if(!dasher) {
        utils::errorMsg("Error creating dasher: exit");
        exit(1);
    }

    if(!dasher->configure(dash_folder, std::string(BASE_NAME), segDuration)){
        utils::errorMsg("Error configuring dasher: exit");
        exit(1);        
    }

    pipe->addFilter(dasherId, dasher);

    return dasher;
}

void addAudioPath(unsigned port, Dasher* dasher, int dasherId, int receiverID, int transmitterID)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int decId = rand();
    int encId = rand();
    int dstReader = rand();
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

    if (!pipe->createPath(port, receiverID, dasherId, port, dstReader, ids)) {
        utils::errorMsg("Error creating audio path");
        return;
    }

    if (!pipe->connectPath(port)){
        utils::errorMsg("Error connecting path");
        pipe->removePath(port);
        return;
    }

    if (!dasher->addSegmenter(dstReader)) {
        utils::errorMsg("Error adding segmenter");
    }

    if (!dasher->setDashSegmenterBitrate(dstReader, OUT_A_BITRATE)) {
        utils::errorMsg("Error setting bitrate to segmenter");
    }
    
    if (!pipe->createPath(BYPASS_AUDIO_PATH, encId, transmitterID, DEFAULT_ID, BYPASS_AUDIO_PATH, std::vector<int>({}))) {
        utils::errorMsg("Error creating audio path");
        return;
    }
    
    if (!pipe->connectPath(BYPASS_AUDIO_PATH)){
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return;
    }

    utils::infoMsg("Audio path created from port " + std::to_string(port));
}

void addVideoPath(unsigned port, Dasher* dasher, int dasherId, int receiverID, int transmitterID,
                    VCodecType codec, int nQ, int maxBitRate)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int decId = 7000;
    int resId = 2000;
    int encId = 1000;
    int dstReader = 3000;
    int slavePathId = 8000;

    std::vector<int> ids({decId, resId, encId});
    std::vector<int> slaveId;

    std::string sessionId;
    std::string sdp;

    VideoDecoderLibav *decoder;
    VideoResampler *resampler;
    VideoEncoderX264 *encoderX264;
    VideoEncoderX265 *encoderX265;

    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);

    resampler = new VideoResampler();
    pipe->addFilter(resId, resampler);
    resampler->configure(1280, 720, 0, YUV420P);

    switch (codec) {
        case H264:
            encoderX264 = new VideoEncoderX264();
            pipe->addFilter(encId, encoderX264);
            encoderX264->configure(maxBitRate, 25, 25, 25, 4, true, "superfast");
            utils::infoMsg("Master reader: " + std::to_string(dstReader));
            break;
        case H265:
            encoderX265 = new VideoEncoderX265();
            pipe->addFilter(encId, encoderX265);
            encoderX265->configure(maxBitRate, 25, 25, 25, 4, true, "superfast");
            utils::infoMsg("Master reader: " + std::to_string(dstReader));
            break;
        default:
            utils::errorMsg("Only H264 and H265 are supported... exiting...");
            exit(1);
            break;
    }

    if (!pipe->createPath(port, receiverID, dasherId, port, dstReader, ids)) {
        utils::errorMsg("Error creating video path");
        return;
    }  

    if (!pipe->connectPath(port)){
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return;
    }

    if (!dasher->addSegmenter(dstReader)) {
        utils::errorMsg("Error adding segmenter");
    }

    if (!dasher->setDashSegmenterBitrate(dstReader, maxBitRate*1000)) {
        utils::errorMsg("Error setting bitrate to segmenter");
    } 

    for (int n = 1; n < nQ; n++) {
        resId += n;
        encId += n;
        dstReader += n;
        slavePathId += n;
        slaveId = {resId, encId};

        resampler = new VideoResampler();
        pipe->addFilter(resId, resampler);
        resampler->configure(1280/2, 720/2, 0, YUV420P);

        switch (codec) {
            case H264:
                encoderX264 = new VideoEncoderX264();
                pipe->addFilter(encId, encoderX264);
                encoderX264->configure(maxBitRate/(n*2), 25, 25, 25, 4, true, "superfast");
                break;
            case H265:
                encoderX265 = new VideoEncoderX265();
                pipe->addFilter(encId, encoderX265);
                encoderX265->configure(maxBitRate/(n*2), 25, 25, 25, 4, true, "superfast");
                break;
            default:
                utils::errorMsg("Only H264 and H265 are supported... exiting...");
                exit(1);
                break;
        }

        if (!pipe->createPath(slavePathId, decId, dasherId, -1, dstReader, slaveId)) {
            utils::errorMsg("Error creating video path");
            return;
        }

        if (!pipe->connectPath(slavePathId)) {
            utils::errorMsg("Error connecting video path");
            return;
        }

        utils::infoMsg("Slave reader: " + std::to_string(dstReader));

        if (!dasher->addSegmenter(dstReader)) {
            utils::errorMsg("Error adding segmenter");
        }
        if (!dasher->setDashSegmenterBitrate(dstReader, maxBitRate*1000/(n*2))) {
            utils::errorMsg("Error setting bitrate to segmenter");
        } 
    }
    
    if (!pipe->createPath(BYPASS_VIDEO_PATH, encId, transmitterID, DEFAULT_ID, BYPASS_VIDEO_PATH, std::vector<int>({}))) {
        utils::errorMsg("Error creating video path");
        return;
    }
    
    if (!pipe->connectPath(BYPASS_VIDEO_PATH)){
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

bool addRTSPsession(std::string rtspUri, Dasher* dasher, int dasherId,
                    SourceManager *receiver, int receiverID, int transmitterID,
                    VCodecType codec, int nQ, int maxBitRate)
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
            addVideoPath(subsession->clientPortNum(), dasher, dasherId, receiverID, transmitterID, codec, nQ, maxBitRate);
        } else if (medium.compare("audio") == 0){
            addAudioPath(subsession->clientPortNum(), dasher, dasherId, receiverID, transmitterID);
        }
    }

    return true;
}

void usage(){
    utils::infoMsg("usage: \n\r \
        testdash -v <RTP input video port> -a <RTP input audio port> -r <input RTSP URI> -c <socket control port> -nvq <number of output video qualities> -vc <output video codec> -f <dash folder> -b <max video bit rate> -s <segment duration> \
        \n INPUTS: RTP or RTSP \n QUALITIES: from 1 to "+std::to_string(MAX_VIDEO_QUALITIES)+"                      \
        \n OUTPUT VIDEO CODECS: H264 or H265                                                                        \
        \n FOLDER: specify system folder where to write DASH MPD, INIT and SEGMENTS files.                          \
    ");
}

bool publishRTSPSession(std::vector<int> readers, SinkManager *transmitter)
{
    std::string sessionId;

    sessionId = "mpegts";
    utils::infoMsg("Adding plain RTP session...");
    if (!transmitter->addRTSPConnection(readers, 2, STD_RTP, sessionId)){
        return false;
    }
    
    return true;
}


//TODO: Define width and height of the master stream and fps
//      define each parameter per stream
int main(int argc, char* argv[])
{
    int vPort = 0;
    int aPort = 0;
    int cPort = 7777;
    int numVidQ = DEFAULT_OUTPUT_VIDEO_QUALITIES;
    int transmitterID = 1024;
    
    std::string vCodec = V_CODEC;
    VCodecType codec;
    std::string dFolder = DASH_FOLDER;
    int maxVideoBitRate = DEFAULT_FIRST_VIDEO_QUALITY;
    int segDuration = SEG_DURATION;
    std::string ip;
    std::string rtspUri = "none";
    Dasher* dasher = NULL;
    int dasherId = 4000;
    std::vector<int> readers;

    int receiverID = rand();

    SourceManager* receiver = NULL;
    SinkManager* transmitter = NULL;
    PipelineManager *pipe;

    utils::setLogLevel(INFO);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-v")==0) {
            vPort = std::stoi(argv[i+1]);
            utils::infoMsg("configuring video input port: " + std::to_string(vPort));
        } else if (strcmp(argv[i],"-a")==0) {
            aPort = std::stoi(argv[i+1]);
            utils::infoMsg("configuring audio input port: " + std::to_string(aPort));
        } else if (strcmp(argv[i],"-r")==0) {
            rtspUri = argv[i+1];
            utils::infoMsg("configuring input RTSP URI: " + rtspUri);
            utils::infoMsg("Ignoring any audio or video input port, just RTSP inputs");
        } else if (strcmp(argv[i],"-c")==0) {
            cPort = std::stoi(argv[i+1]);
            utils::infoMsg("configuring control port: " + std::to_string(cPort));
        } else if (strcmp(argv[i],"-nvq")==0) {
            numVidQ = std::stoi(argv[i+1]);
            utils::infoMsg("configuring number of video qualities: " + std::to_string(numVidQ));
        } else if (strcmp(argv[i],"-vc")==0) {
            vCodec = argv[i+1];
            utils::infoMsg("configuring output video codec: " + vCodec);
        } else if (strcmp(argv[i],"-f")==0) {
            dFolder = argv[i+1];
            utils::infoMsg("configuring dash folder: " + dFolder);
        } else if (strcmp(argv[i],"-b")==0) {
            maxVideoBitRate = std::stoi(argv[i+1]);
            utils::infoMsg("configuring maximum video bitrate: " + std::to_string(maxVideoBitRate));
        } else if (strcmp(argv[i],"-s")==0) {
            segDuration = std::stoi(argv[i+1]);
            utils::infoMsg("configuring dash segments duration: " + std::to_string(segDuration));
        }
    }

    if (vPort == 0 && aPort == 0 && rtspUri.length() == 4){
        utils::errorMsg("invalid parameters");
        usage();
        return 1;
    }

    if (numVidQ < 1 || numVidQ > MAX_VIDEO_QUALITIES){
        utils::errorMsg("Number of output video qualities can be 1, 2 or 3");
        usage();
        return 1;
    }

    if (vCodec == "H264") {
        codec = H264;
    } else if (vCodec == "H265") {
        codec = H265;
    } else {
        utils::errorMsg("Only H264 and H265 ouput codecs are supported");
        usage();
        return 1;
    }

    if (access(dFolder.c_str(), W_OK) != 0) {
        utils::errorMsg("Error configuring Dasher: provided folder is not writable or does not exist");
        usage();
        return false;
    }

    utils::infoMsg("Running testDasher:                                                      \
        \n\t\t\t\t - video receiver port: " + std::to_string(vPort) + "                      \
        \n\t\t\t\t - audio receiver port: " + std::to_string(aPort) + "                      \
        \n\t\t\t\t - input RTSP URI: " + rtspUri + "                                         \
        \n\t\t\t\t - output video codec: " + vCodec + "                                      \
        \n\t\t\t\t - number of output video qualities: " + std::to_string(numVidQ) + "       \
        \n\t\t\t\t - dash folder: " + dFolder + "                                            \
    ");

    pipe = Controller::getInstance()->pipelineManager();

    receiver = new SourceManager();
    pipe->addFilter(receiverID, receiver);
    
    transmitter = SinkManager::createNew();
    if (!transmitter){
        utils::errorMsg("RTSPServer constructor failed");
        return 1;
    }

    pipe->addFilter(transmitterID, transmitter);

    dasher = setupDasher(dasherId, dFolder, segDuration);
    
    signal(SIGINT, signalHandler);

    if (vPort != 0 && rtspUri.length() == 4){
        addVideoSDPSession(vPort, receiver);
        addVideoPath(vPort, dasher, dasherId, receiverID, transmitterID, codec, numVidQ, maxVideoBitRate);
    }

    if (aPort != 0 && rtspUri.length() == 4){
        addAudioSDPSession(aPort, receiver);
        addAudioPath(aPort, dasher, dasherId, receiverID, transmitterID);
    }

    if (rtspUri.length() > 4){
        if (!addRTSPsession(rtspUri, dasher, dasherId, receiver, receiverID, transmitterID, codec, numVidQ, maxVideoBitRate)){
            utils::errorMsg("Couldn't start rtsp client session!");
            usage();
            return 1;
        }
    }
    
    for (auto it : pipe->getPaths()) {
        if (it.second->getDstReaderID() == BYPASS_VIDEO_PATH || it.second->getDstReaderID() == BYPASS_AUDIO_PATH){
            readers.push_back(it.second->getDstReaderID());
        }
    }
    
    if (!publishRTSPSession(readers, transmitter)){
        utils::errorMsg("Failed adding RTSP sessions!");
        return 1;
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

    ctrl->destroyInstance();
    utils::infoMsg("Controlled deleted");
    pipe->destroyInstance();
    utils::infoMsg("Pipe deleted");
    
    return 0;
}