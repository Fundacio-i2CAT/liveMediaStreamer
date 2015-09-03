#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/modules/audioMixer/AudioMixer.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#include "../src/modules/videoMixer/VideoMixer.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
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
#define A_CODEC "OPUS"
#define A_BANDWITH 128
#define A_TIME_STMP_FREQ 48000
#define A_CHANNELS 2

#define OUT_A_CODEC AAC
#define OUT_A_FREQ 48000
#define OUT_A_BITRATE 192000

#define RETRIES 10

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
    encoder->configure(2000, 25, 25, 0, 4, true, "superfast");
    
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
    
    return true;
}

void logState()
{
    size_t totalPathLostBlocs;
    PipelineManager* pipe = PipelineManager::getInstance();
    std::string logger;
    BaseFilter* f;

    MediaSubsession* subsession;
    SCSSubsessionStats* scsss;  
    unsigned numPacketsReceived = 0, numPacketsExpected = 0;
    unsigned secsDiff = 0;
    int usecsDiff = 0;
    double measurementTime = 0;
    double packetLossFraction = 0;
    double totalGapsMS;

    RTPConnection* rtpConn;
    RTSPConnection* rtspConn;

    for (auto it : pipe->getPaths()) {
        std::vector<int> pFilters = it.second->getFilters();
        logger += "|PATHID|"+std::to_string(it.first);
        // PATH DELAY and LOST BLOCS
        f = pipe->getFilter(it.second->getDestinationFilterID());
        if (f) {
            logger += "|avgDelay-"+std::to_string(it.first)+"|"+std::to_string(f->getAvgReaderDelay(it.second->getDstReaderID()).count());
            totalPathLostBlocs = 0;
            totalPathLostBlocs += f->getLostBlocs(it.second->getDstReaderID());
            for (auto itt : pFilters) {
                f =  pipe->getFilter(itt);
                if (f){
                    totalPathLostBlocs += f->getLostBlocs(DEFAULT_ID);
                }
            }
            logger += "|lostBlocs-"+std::to_string(it.first)+"|"+std::to_string(totalPathLostBlocs);
        } else {
            utils::warningMsg("[PipelineManager::getStateEvent] Path filter does not exist. Ambiguous situation! Better pray Jesus...");
        }
    }
    for (auto it : pipe->getFilters()) {
        if(it.second->getType() == RECEIVER){
            for (auto itt : dynamic_cast<SourceManager*>(it.second)->getSessions()) {
                if (!itt.second->getScs()->session) {
                    continue;
                }
                MediaSubsessionIterator iter(*(itt.second->getScs()->session));
                while ((subsession = iter.next()) != NULL) {
                    std::string medium = subsession->mediumName();
                    logger += "|RXmediaType|"+medium;
                    if((scsss = itt.second->getScs()->getSubsessionStats(subsession->clientPortNum())) != NULL){
                        numPacketsReceived = scsss->getTotNumPacketsReceived();
                        numPacketsExpected = scsss->getTotNumPacketsExpected();
                        secsDiff  = scsss->getMeasurementEndTime().tv_sec - scsss->getMeasurementStartTime().tv_sec;
                        usecsDiff = scsss->getMeasurementEndTime().tv_usec - scsss->getMeasurementStartTime().tv_usec;
                        measurementTime  = secsDiff + usecsDiff/1000000.0;
                        // BITRATE
                        if ( scsss->getKbitsPerSecondMax() == 0) {
                            logger += "|"+medium+"RXavgBitRateInKbps|0";
                        } else {
                            logger += "|"+medium+"RXavgBitRateInKbps|"+std::to_string(measurementTime == 0.0 ? 0.0 : 8*scsss->getKBytesTotal()/measurementTime);
                        }
                        // PACKET LOSS
                        packetLossFraction = numPacketsExpected == 0 ? 1.0 : 1.0 - numPacketsReceived/(double)numPacketsExpected;
                        if (packetLossFraction < 0.0){
                            logger += "|"+medium+"RXavgPacketLossPercentage|0.0";  
                        } else {
                            logger += "|"+medium+"RXavgPacketLossPercentage|"+std::to_string(100*packetLossFraction);
                        }
                        // INTER PACKET GAP
                        totalGapsMS = scsss->getTotalGaps().tv_sec*1000.0 + scsss->getTotalGaps().tv_usec/1000.0;
                        logger += "|"+medium+"RXavgInterPacketGapInMiliseconds|"+std::to_string(numPacketsReceived == 0 ? 0.0 : totalGapsMS/numPacketsReceived);
                        // JITTER 
                        logger += "|"+medium+"RXcurJitterInMicroseconds|"+std::to_string(scsss->getJitter());
                    }
                }
            }
        }
        if(it.second->getType() == TRANSMITTER){
            size_t count = 0;
            for (auto itt : dynamic_cast<SinkManager*>(it.second)->getConnections()) {
                if ((rtspConn = dynamic_cast<RTSPConnection*>(itt.second))){
                    count = 0;
                    logger += "|TX_URI|"+rtspConn->getURI();
                    for (auto iter : itt.second->getConnectionRTCPInstanceMap()) {
                        logger += "|TX_SSRC-"+std::to_string(++count)+"|"+std::to_string(iter.second->getSSRC());
                        logger += "|TXavgBitrateInKbps-"+std::to_string(count)+"|"+std::to_string(iter.second->getAvgBitrate());
                        logger += "|TXpacketLossRatio-"+std::to_string(count)+"|"+std::to_string(iter.second->getPacketLossRatio());
                        logger += "|TXjitterInMicroseconds-"+std::to_string(count)+"|"+std::to_string(iter.second->getJitter());
                        logger += "|TXroundTripDelayMilliseconds-"+std::to_string(count)+"|"+std::to_string(iter.second->getRoundTripDelay());
                    }
                } else if ((rtpConn = dynamic_cast<RTPConnection*>(itt.second))){
                    count = 0;
                    logger += "|TXip|"+rtpConn->getIP();
                    logger += "|TXport|"+std::to_string(rtpConn->getPort());
                    for (auto iter : itt.second->getConnectionRTCPInstanceMap()) {
                        logger += "|TX_SSRC-"+std::to_string(++count)+"|"+std::to_string(iter.second->getSSRC());
                        logger += "|TXavgBitrateInKbps-"+std::to_string(count)+"|"+std::to_string(iter.second->getAvgBitrate());
                        logger += "|TXpacketLossRatio-"+std::to_string(count)+"|"+std::to_string(iter.second->getPacketLossRatio());
                        logger += "|TXjitterInMicroseconds-"+std::to_string(count)+"|"+std::to_string(iter.second->getJitter());
                        logger += "|TXroundTripDelayMilliseconds-"+std::to_string(count)+"|"+std::to_string(iter.second->getRoundTripDelay());
                    }
                }
            }               
        }   
    }
    std::cout << logger <<"|"<< std::endl;
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
    std::chrono::duration<double> diff;
    auto now = std::chrono::system_clock::now();
    auto prev = std::chrono::system_clock::now();
    unsigned seconds = 0;

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
        if (transmitter->addRTPConnection(readers, rand(), ip, port, STD_RTP)) {
            utils::infoMsg("added connection for " + ip + ":" + std::to_string(port));
        }
    }

    Controller* ctrl = Controller::getInstance();

    if (!ctrl->createSocket(cPort)) {
        exit(1);
    }

    while (run) {
        now = std::chrono::system_clock::now();
        diff = now - prev;
        if(diff.count() >= 1){
            utils::infoMsg("NEW STATS LOG AT SECOND "+std::to_string(++seconds));
            logState();
            prev = now;
        }

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
    utils::infoMsg("Controller deleted");
    pipe->destroyInstance();
    utils::infoMsg("Pipe deleted");
    
    return 0;
}
