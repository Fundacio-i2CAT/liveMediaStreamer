#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#include "../src/modules/videoSplitter/VideoSplitter.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/receiver/SourceManager.hh"
#include "../src/modules/transmitter/SinkManager.hh"
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
#define SPLIT_CHANNELS 4
#define SPLIT_WIDTH 1280
#define SPLIT_HEIGHT 720


bool run = true;
int layer = 0;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
}

bool setupSplitter(int splitterId, int transmitterID) 
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    VideoEncoderX264* encoder;
    VideoResampler* resampler;

    VideoSplitter* splitter;

    splitter = VideoSplitter::createNew(std::chrono::microseconds(40000));
    pipe->addFilter(splitterId, splitter);

    int encId = 300;
    int resId = 400;
    int pathId = 500;

    for(int it = 1; it <= SPLIT_CHANNELS; it++){
        std::vector<int> ids = {resId + it, encId + it};
        
        resampler = new VideoResampler();
        pipe->addFilter(resId + it, resampler);
        resampler->configure(0, 0, 0, YUV420P);

        encoder = new VideoEncoderX264();
        //bitrate, fps, gop, lookahead, threads, annexB, preset
        encoder->configure(2000, 25, 25, 25, 4, 4, true, "superfast");
        pipe->addFilter(encId + it, encoder);

        if (!pipe->createPath(pathId + it, splitterId, transmitterID, it, it, ids)) {
            utils::errorMsg("Error creating path");
            return false;
        }
        
        if (!pipe->connectPath(pathId + it)) {
            utils::errorMsg("Error creating path");
            pipe->removePath(pathId + it);
            return false;
        }

        float w = 0.5;
        float h = 0.5;
        float x = ((it-1)%2)*0.5;
        float y = ((it>>1)%2)*0.5;
        
        splitter->configCrop(it,w,h,x,y);

        utils::errorMsg("[TESTVIDEOSPLITTER] Crop config "+std::to_string(it)+":");
        utils::errorMsg("[TESTVIDEOSPLITTER] Wwidth:"+std::to_string(w)+", height:"+std::to_string(h)+", POS.X:"+std::to_string(x)+", POS.Y:"+std::to_string(y));

        ids.clear();
    }
    return true;
}

void addVideoPath(unsigned port, int receiverId, int splitterId)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int resId = rand();
    int decId = rand();

    std::vector<int> ids = {decId, resId};

    std::string sessionId;
    std::string sdp;

    VideoDecoderLibav *decoder;
    VideoResampler *resampler;

    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);

    resampler = new VideoResampler();
    resampler->configure(0, 0, 0, RGB24);
    pipe->addFilter(resId, resampler);

    if (!pipe->createPath(port, receiverId, splitterId, port, -1, ids)) {
        utils::errorMsg("Error creating audio path");
        return;
    }

    if (!pipe->connectPath(port)) {
        utils::errorMsg("Error connecting path");
        pipe->removePath(port);
        return;
    }

    utils::infoMsg("Video path created");
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

bool addRTSPsession(std::string rtspUri, SourceManager *receiver, int receiverId, int splitterId)
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
            addVideoPath(subsession->clientPortNum(), receiverId, splitterId);
        } else {
            utils::warningMsg("Only video is supported in this test. Ignoring audio streams");
        }
    }

    return true;
}

bool publishRTSPSession(std::vector<int> readers, SinkManager *transmitter)
{
    std::string sessionId;

    utils::infoMsg("Adding plain RTP session...");
    for (auto it : readers)
    {
        if(it ==0){
            continue;
        } 
        std::vector<int> out;
        sessionId = "plainrtp" + std::to_string(it);
        out.push_back(it);
        if (!transmitter->addRTSPConnection(out, it, STD_RTP, sessionId)){
            return false;
        }
    }
    /*sessionId = "mpegts";
    utils::infoMsg("Adding plain MPEGTS session...");
    if (!transmitter->addRTSPConnection(readers, 2, MPEGTS, sessionId)){
        return false;
    }*/

    return true;
}

int main(int argc, char* argv[])
{
    std::vector<int> readers;
    std::string rtspUri;

    int transmitterId = 11;
    int receiverId = 10;
    int splitterId  = 15;
    
    int def_witdth = SPLIT_WIDTH;
    int def_height = SPLIT_HEIGHT;

    int port = 0;
    int vPort = 0;
    std::string ip;

    SinkManager* transmitter = NULL;
    SourceManager* receiver = NULL;
    PipelineManager *pipe;

    utils::setLogLevel(INFO);
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-v") == 0){
            vPort = std::stoi(argv[i+1]);
            utils::infoMsg("video input port: " + std::to_string(vPort));  
        } else if (strcmp(argv[i],"-w")==0) {
            def_witdth = std::stoi(argv[i+1]);
        } else if (strcmp(argv[i],"-h")==0) {
            def_height = std::stoi(argv[i+1]);
        } else if (strcmp(argv[i],"-r")==0) {
            rtspUri = argv[i+1];
            utils::infoMsg("input RTSP URI: " + rtspUri);
            utils::infoMsg("Ignoring any audio or video input port, just RTSP inputs");
        } else if (strcmp(argv[i],"-d")==0) {
            ip = argv[i + 1];
            utils::infoMsg("destination IP: " + ip);
        } else if (strcmp(argv[i],"-P")==0) {
            port = std::stoi(argv[i+1]);
            utils::infoMsg("destination port: " + std::to_string(port));
        }
    }
    utils::infoMsg("input WIDTH: " + std::to_string(def_witdth) + " and input HEIGHT: " + std::to_string(def_height));

    if (vPort == 0  && rtspUri.empty()) { 
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

    pipe->addFilter(transmitterId, transmitter);
    pipe->addFilter(receiverId, receiver);

    setupSplitter(splitterId, transmitterId);

    signal(SIGINT, signalHandler);

    for (auto it : pipe->getPaths()) {
        readers.push_back(it.second->getDstReaderID());
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
        for (auto it : readers)
        {
            if(it == 0){
                continue;
            } 
            std::vector<int> out;
            out.push_back(it);
            if (it == 1 && transmitter->addRTPConnection(out, rand(), ip, port, STD_RTP)) {
                utils::infoMsg("added connection for " + ip + ":" + std::to_string(port));
            }
        }    
    }

    if (vPort > 0 && rtspUri.length() == 0){
        addVideoSDPSession(vPort, receiver);
        addVideoPath(vPort, receiverId, splitterId);
    } else {
        if (!addRTSPsession(rtspUri, receiver, receiverId, splitterId)){
            utils::errorMsg("Couldn't start rtsp client session!");
            return 1;
        }
    }

    while(run) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    Controller::destroyInstance();
    PipelineManager::destroyInstance();
    
    return 0;
}