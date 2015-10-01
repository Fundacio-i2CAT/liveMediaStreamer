#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/modules/audioMixer/AudioMixer.hh"
#include "../src/modules/receiver/SourceManager.hh"
#include "../src/modules/transmitter/SinkManager.hh"
#include "../src/Controller.hh"
#include "../src/Utils.hh"

#include <csignal>
#include <vector>
#include <string>

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

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
}

bool setupMixer(int mixerId, int transmitterId) 
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    AudioMixer* mixer;
    AudioEncoderLibav* encoder;

    int encId = rand();
    int pathId = rand();

    std::vector<int> ids = {encId};

    //NOTE: Adding decoder to pipeManager and handle worker
    mixer = new AudioMixer();
    pipe->addFilter(mixerId, mixer);

    encoder = new AudioEncoderLibav();
    if (!encoder->configure(OUT_A_CODEC, A_CHANNELS, OUT_A_FREQ, OUT_A_BITRATE)) {
        utils::errorMsg("Error configuring audio encoder. Check provided parameters");
        return false;
    }

    pipe->addFilter(encId, encoder);

    if (!pipe->createPath(pathId, mixerId, transmitterId, -1, -1, ids)) {
        utils::errorMsg("Error creating path");
        return false;
    }

    if (!pipe->connectPath(pathId)) {
        utils::errorMsg("Error creating path");
        pipe->removePath(pathId);
        return false;
    }

    return true;
}

void addAudioPath(unsigned port, int receiverId, int mixerId)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int decId = rand();
    std::vector<int> ids({decId});

    AudioDecoderLibav *decoder;

    decoder = new AudioDecoderLibav();
    pipe->addFilter(decId, decoder);

    if (!pipe->createPath(port, receiverId, mixerId, port, port, ids)) {
        utils::errorMsg("Error creating audio path");
        return;
    }

    if (!pipe->connectPath(port)) {
        utils::errorMsg("Error connecting path");
        pipe->removePath(port);
        return;
    }

    utils::infoMsg("Audio path created from port " + std::to_string(port));
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

        if (medium.compare("audio") == 0) {
            addAudioPath(subsession->clientPortNum(), receiverId, mixerId);
        } else {
            utils::warningMsg("Only audio is supported in this test. Ignoring video streams");
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

    int transmitterId = rand();
    int receiverId = rand();
    int mixerId = rand();

    int port;

    SinkManager* transmitter = NULL;
    SourceManager* receiver = NULL;
    PipelineManager *pipe;

    utils::setLogLevel(INFO);

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i],"-a") == 0) {
            port = std::stoi(argv[i+1]);
            ports.push_back(port);
            utils::infoMsg("audio input port: " + std::to_string(port));
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

    pipe->addFilter(transmitterId, transmitter);
    pipe->addFilter(receiverId, receiver);

    if (!setupMixer(mixerId, transmitterId)) {
        utils::errorMsg("Mixer setup failed");
        return 1;
    }

    signal(SIGINT, signalHandler);

    for (auto it : pipe->getPaths()) {
        readers.push_back(it.second->getDstReaderID());
    }

    if (!publishRTSPSession(readers, transmitter)){
        utils::errorMsg("Failed adding RTSP sessions!");
        return 1;
    }

    for (auto p : ports) {
        addAudioSDPSession(p, receiver);
        addAudioPath(p, receiverId, mixerId);
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
    
    Controller::destroyInstance();
    PipelineManager::destroyInstance();

    return 0;
}
