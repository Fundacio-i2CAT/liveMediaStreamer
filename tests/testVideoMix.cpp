#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#include "../src/modules/videoMixer/VideoMixer.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
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

#define RETRIES 60
#define MIX_WIDTH 640
#define MIX_HEIGHT 360
#define MIX_CHANNELS 8

bool run = true;
int layer = 0;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
    Controller::getInstance()->pipelineManager()->stop();
    Controller::destroyInstance();
    PipelineManager::destroyInstance();
    exit(0);
}

void setupMixer(int mixerId, int transmitterId) 
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    VideoMixer* mixer;
    VideoEncoderX264* encoder;
    VideoResampler* resampler;

    Path* path;

    int encId = rand();
    int resId = rand();
    int pathId = rand();

    std::vector<int> ids = {resId, encId};

    //NOTE: Adding decoder to pipeManager and handle worker
    mixer = VideoMixer::createNew(MIX_CHANNELS, MIX_WIDTH, MIX_HEIGHT, std::chrono::microseconds(40000));
    pipe->addFilter(mixerId, mixer);

    //NOTE: Adding resampler to pipeManager and handle worker
    resampler = new VideoResampler();
    pipe->addFilter(resId, resampler);
    resampler->configure(0, 0, 0, YUV420P);

    //NOTE: Adding encoder to pipeManager and handle worker
    encoder = new VideoEncoderX264();
    //bitrate, fps, gop, lookahead, threads, annexB, preset
    // encoder->configure(4000, 25, 25, 25, 4, true, "superfast");
    pipe->addFilter(encId, encoder);

    path = pipe->createPath(mixerId, transmitterId, -1, -1, ids);
    pipe->addPath(pathId, path);
    pipe->connectPath(path);
}

void addVideoPath(unsigned port, int receiverId, int mixerId)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int resId = rand();
    int decId = rand();

    std::vector<int> ids = {decId, resId};

    std::string sessionId;
    std::string sdp;

    VideoDecoderLibav *decoder;
    VideoResampler *resampler;
    VideoMixer* mixer;

    Path *path;

    //NOTE: Adding decoder to pipeManager and handle worker
    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);

    //NOTE: Adding resampler to pipeManager and handle worker
    resampler = new VideoResampler();
    resampler->configure(MIX_WIDTH*0.5, MIX_HEIGHT*0.5, 0, RGB24);
    pipe->addFilter(resId, resampler);

    path = pipe->createPath(receiverId, mixerId, port, port, ids);
    pipe->addPath(port, path);
    pipe->connectPath(path);

    mixer = dynamic_cast<VideoMixer*>(pipe->getFilter(mixerId));

    if (!mixer) {
        utils::errorMsg("Error getting videoMixer from pipe");
        return;
    }

    mixer->configChannel(port, 0.5, 0.5, rand()%50/100.0, rand()%50/100.0, layer++, true, 1.0);

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

bool addRTSPsession(std::string rtspUri, SourceManager *receiver, int receiverId, int mixerId)
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

        if (medium.compare("video") == 0) {
            addVideoPath(subsession->clientPortNum(), receiverId, mixerId);
        } else {
            utils::warningMsg("Only video is supported in this test. Ignoring audio streams");
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
    std::vector<int> ports;
    std::vector<std::string> uris;
    std::vector<int> readers;

    std::string rtspUri;

    int transmitterID = rand();
    int receiverId = rand();
    int mixerId = rand();

    int port;

    SinkManager* transmitter = NULL;
    SourceManager* receiver = NULL;
    PipelineManager *pipe;

    utils::setLogLevel(INFO);

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i],"-v") == 0) {
            port = std::stoi(argv[i+1]);
            ports.push_back(port);
            utils::infoMsg("video input port: " + std::to_string(port));
        } else if (strcmp(argv[i],"-r")==0) {
            uris.push_back(argv[i+1]);
            utils::infoMsg("input RTSP URI: " + rtspUri);
        }
    }
 
    if (ports.empty() && rtspUri.empty()) { 
        utils::errorMsg("Usage: -v <port> -r <uri>");
        return 1;
    }

    receiver = new SourceManager();
    pipe = Controller::getInstance()->pipelineManager();

    transmitter = SinkManager::createNew();
    
    if (!transmitter) {
        utils::errorMsg("RTSPServer constructor failed");
        return 1;
    }

    pipe->addFilter(transmitterID, transmitter);
    pipe->addFilter(receiverId, receiver);

    setupMixer(mixerId, transmitterID);

    signal(SIGINT, signalHandler);

    for (auto it : pipe->getPaths()) {
        readers.push_back(it.second->getDstReaderID());
    }

    if (!publishRTSPSession(readers, transmitter)){
        utils::errorMsg("Failed adding RTSP sessions!");
        return 1;
    }

    for (auto p : ports) {
        addVideoSDPSession(p, receiver);
        addVideoPath(p, receiverId, mixerId);
    }

    for (auto uri : uris) {
        if (!addRTSPsession(uri, receiver, receiverId, mixerId)){
            utils::errorMsg("Couldn't start rtsp client session!");
            return 1;
        }
    }

    while(run) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }


    return 0;
}
