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
#include "../src/modules/dasher/Dasher.hh"
#include "../src/AudioFrame.hh"
#include "../src/Controller.hh"
#include "../src/Utils.hh"
#include "../src/modules/sharedMemory/SharedMemory.hh"
#include <sys/resource.h>

#include <csignal>
#include <vector>
#include <string>

#define MAX_SEGMENTS 6

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

#define MAX_VIDEO_QUALITIES 32
#define DEFAULT_OUTPUT_VIDEO_QUALITIES 3
#define DEFAULT_FIRST_VIDEO_QUALITY 2000

#define SEG_DURATION 4 //sec
#define DASH_FOLDER "/tmp/dash"
#define BASE_NAME "test"

bool run = true;

struct Configuration {
    int width;
    int height;
    int bitrate;
    VCodecType codec;
};

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
}

Dasher* setupDasher(int dasherId, std::string dash_folder, int segDuration, std::string basename)
{
    Dasher* dasher = NULL;

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    dasher = new Dasher();

    if(!dasher) {
        utils::errorMsg("Error creating dasher: exit");
        exit(1);
    }

    if(!dasher->configure(dash_folder, basename, segDuration, 30, 16)){
        utils::errorMsg("Error configuring dasher: exit");
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

    if (!dasher->setDashSegmenterBitrate(dstReader, OUT_A_BITRATE)) {
        utils::errorMsg("Error setting bitrate to segmenter");
    }
    
    utils::infoMsg("Audio path created from port " + std::to_string(port));
}

void addVideoPath(unsigned port, Dasher* dasher, int dasherId, int receiverID,
                    Configuration *config, int numConfig)
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
    resampler->configure(config[0].width, config[0].height, 0, YUV420P);

    switch (config[0].codec) {
        case H264:
            encoderX264 = new VideoEncoderX264();
            pipe->addFilter(encId, encoderX264);
            encoderX264->configure(config[0].bitrate, 25, 25, 0, 4, true, "superfast");
            utils::infoMsg("Master reader: " + std::to_string(dstReader));
            break;
        case H265:
            encoderX265 = new VideoEncoderX265();
            pipe->addFilter(encId, encoderX265);
            encoderX265->configure(config[0].bitrate, 25, 25, 25, 4, true, "superfast");
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

    if (!dasher->setDashSegmenterBitrate(dstReader, config[0].bitrate*1000)) {
        utils::errorMsg("Error setting bitrate to segmenter");
    } 

    for (int n = 1; n < numConfig; n++) {
        resId += n;
        encId += n;
        dstReader += n;
        slavePathId += n;
        slaveId = {resId, encId};

        resampler = new VideoResampler();
        pipe->addFilter(resId, resampler);
        resampler->configure(config[n].width, config[n].height, 0, YUV420P);

        switch (config[n].codec) {
            case H264:
                encoderX264 = new VideoEncoderX264();
                pipe->addFilter(encId, encoderX264);
                encoderX264->configure(config[n].bitrate, 25, 25, 0, 4, true, "superfast");
                break;
            case H265:
                encoderX265 = new VideoEncoderX265();
                pipe->addFilter(encId, encoderX265);
                encoderX265->configure(config[n].bitrate, 25, 25, 25, 4, true, "superfast");
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

        if (!dasher->setDashSegmenterBitrate(dstReader, config[n].bitrate*1000)) {
            utils::errorMsg("Error setting bitrate to segmenter");
        } 
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
                    SourceManager *receiver, int receiverID,
                    Configuration *config, int numConfig)
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

    iter.reset();

    while((subsession = iter.next()) != NULL){
        medium = subsession->mediumName();

        if (medium.compare("video") == 0){
            addVideoPath(subsession->clientPortNum(), dasher, dasherId, receiverID, config, numConfig);
        } else if (medium.compare("audio") == 0){
            addAudioPath(subsession->clientPortNum(), dasher, dasherId, receiverID);
        }
    }

    return true;
}

void usage(){
    utils::infoMsg("usage: \n\r \
        testdash -v <RTP input video port> -a <RTP input audio port> -r <input RTSP URI> -c <socket control port> -f <dash folder> -s <segment duration> -statsfile <output statistics filename> -timeout <secons to wait before closing. 0 means forever> -configfile <configuration file>\
        \n INPUTS: RTP or RTSP \n QUALITIES: from 1 to "+std::to_string(MAX_VIDEO_QUALITIES)+"                      \
        \n FOLDER: specify system folder where to write DASH MPD, INIT and SEGMENTS files.                          \
        \n Each line in the configuration file must contain 'width, height, bitrate (kbps), codec (0:H264 1:H265)'  \
    ");
}

//TODO: Define width and height of the master stream and fps
//      define each parameter per stream
int main(int argc, char* argv[])
{
    int vPort = 0;
    int aPort = 0;
    int cPort = 7777;
    
    std::string vCodec = V_CODEC;
    std::string dFolder = DASH_FOLDER;
    int segDuration = SEG_DURATION;
    std::string ip;
    std::string rtspUri = "none";
    Dasher* dasher = NULL;
    int dasherId = 4000;
    std::vector<int> readers;
    std::string basename = BASE_NAME;

    int receiverID = rand();

    SourceManager* receiver = NULL;
    PipelineManager *pipe;

    utils::setLogLevel(INFO);

    int timeout = 0;
    std::string stats_filename;
    std::string config_filename;

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
        } else if (strcmp(argv[i],"-f")==0) {
            dFolder = argv[i+1];
            utils::infoMsg("configuring dash folder: " + dFolder);
        } else if (strcmp(argv[i],"-s")==0) {
            segDuration = std::stoi(argv[i+1]);
            utils::infoMsg("configuring dash segments duration: " + std::to_string(segDuration));
        } else if (strcmp(argv[i],"-statsfile")==0) {
            stats_filename = argv[i+1];
            utils::infoMsg("output stats filename: " + stats_filename);
        } else if (strcmp(argv[i],"-timeout")==0) {
            timeout = std::stoi(argv[i+1]);
            utils::infoMsg("timeout: " + std::to_string(timeout) + "s.");
        } else if (strcmp(argv[i],"-configfile")==0) {
            config_filename = argv[i+1];
            utils::infoMsg("config filename: " + config_filename);
        } else if (strcmp(argv[i],"-basename")) {
            basename = argv[i+1];
            utils::infoMsg("basename: " + basename);
        }
    }

    if (vPort == 0 && aPort == 0 && rtspUri.length() == 4){
        utils::errorMsg("invalid parameters");
        usage();
        return 1;
    }

    if (config_filename.empty()) {
        utils::errorMsg("Please specify a configuration file");
        return 1;
    }
    Configuration config[MAX_VIDEO_QUALITIES];
    int numConfig = 0;
    FILE *cf = fopen (config_filename.c_str(), "rt");
    if (!cf) {
        utils::errorMsg("Could not open config file " + config_filename);
        return 1;
    }
    int codecType;
    while (fscanf (cf, "%d, %d, %d, %d",
                &config[numConfig].width,
                &config[numConfig].height,
                &config[numConfig].bitrate,
                &codecType) == 4) {
        config[numConfig].codec = (VCodecType)codecType;
        if (config[numConfig].codec != H264 &&
            config[numConfig].codec != H265) {
            utils::errorMsg("Only H264 and H265 are supported (codec type 0 and 1)");
            return 1;
        }
        numConfig++;
    }
    fclose(cf);

    if (numConfig < 1 || numConfig > MAX_VIDEO_QUALITIES){
        utils::errorMsg("Number of output video qualities must be between 1 and " + std::to_string(MAX_VIDEO_QUALITIES));
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
        \n\t\t\t\t - number of output configurations: " + std::to_string(numConfig) + "      \
        \n\t\t\t\t - dash folder: " + dFolder + "                                            \
        \n\t\t\t\t - basename: " + basename + "                                              \
    ");

    pipe = Controller::getInstance()->pipelineManager();

    receiver = new SourceManager();
    pipe->addFilter(receiverID, receiver);
    
    dasher = setupDasher(dasherId, dFolder, segDuration, basename);
    
    signal(SIGINT, signalHandler);

    if (vPort != 0 && rtspUri.length() == 4){
        addVideoSDPSession(vPort, receiver);
        addVideoPath(vPort, dasher, dasherId, receiverID, config, numConfig);
    }

    if (aPort != 0 && rtspUri.length() == 4){
        addAudioSDPSession(aPort, receiver);
        addAudioPath(aPort, dasher, dasherId, receiverID);
    }

    if (rtspUri.length() > 4){
        if (!addRTSPsession(rtspUri, dasher, dasherId, receiver, receiverID, config, numConfig)){
            utils::errorMsg("Couldn't start rtsp client session!");
            usage();
            return 1;
        }
    }

    Controller* ctrl = Controller::getInstance();

    if (!ctrl->createSocket(cPort)) {
        exit(1);
    }

    struct rusage usage0;
    getrusage(RUSAGE_SELF, &usage0);

    // Running time (wall clock)
    struct timespec rtime0str, rtime1str;
    clock_gettime (CLOCK_REALTIME, &rtime0str);
    while (run) {
        if (!ctrl->listenSocket()) {
            if (timeout) {
                clock_gettime (CLOCK_REALTIME_COARSE, &rtime1str);
                if (rtime1str.tv_sec > rtime0str.tv_sec + timeout) {
                    run = false;
                }
            }
            continue;
        }

        if (!ctrl->readAndParse()) {
            utils::errorMsg("Controller failed to read and parse the incoming event data");
            continue;
        }

        ctrl->processRequest();
    }

    struct rusage usage1;
    getrusage(RUSAGE_SELF, &usage1);

    if (!stats_filename.empty()) {
        Jzon::Object pipe_state;
        pipe->getStateEvent(NULL, pipe_state);

        FILE *f = fopen(stats_filename.c_str(), "wt");

        const Jzon::Array &paths = pipe_state.Get("paths");
        int inDelay = 0, inLosses = 0, inCount = 0;
        for (Jzon::Array::const_iterator it = paths.begin(); it != paths.end(); ++it) {
            int destinationFilter = (*it).Get("destinationFilter").ToInt();
            if (destinationFilter == dasherId) {
                // Input path: Receiver -> Dasher
                inDelay += (*it).Get("avgDelay").ToInt();
                inLosses += (*it).Get("lostBlocs").ToInt();
                inCount++;
            }
        }
        inDelay /= inCount;

        fprintf(f, "%d, %d, ", inDelay, inLosses);

        float inBitrate = 0, inPacketLoss = 0;
        const Jzon::Array &filters = pipe_state.Get("filters");
        for (Jzon::Array::const_iterator it = filters.begin(); it != filters.end(); ++it) {
            std::string type = (*it).Get("type").ToString();
            if (type.compare("receiver") == 0) {
                const Jzon::Array &sessions = (*it).Get("sessions");
                for (Jzon::Array::const_iterator it2 = sessions.begin(); it2 != sessions.end(); ++it2) {
                    const Jzon::Array subs = (*it2).Get("subsessions").AsArray();
                    for (Jzon::Array::const_iterator it3 = subs.begin(); it3 != subs.end(); ++it3) {
                        inBitrate += (*it3).Get("avgBitRateInKbps").ToFloat();
                        inPacketLoss += (*it3).Get("avgPacketLossPercentage").ToFloat();
                    }
                }
            }
        }

        fprintf(f, "%f, %f, ", inBitrate, inPacketLoss);

        // User time and System time, in microseconds
        int utime0 = usage0.ru_utime.tv_usec + usage0.ru_utime.tv_sec * 1000000;
        int stime0 = usage0.ru_stime.tv_usec + usage0.ru_stime.tv_sec * 1000000;
        int utime1 = usage1.ru_utime.tv_usec + usage1.ru_utime.tv_sec * 1000000;
        int stime1 = usage1.ru_stime.tv_usec + usage1.ru_stime.tv_sec * 1000000;

        // Total running time, in microseconds
        int rtime0 = rtime0str.tv_nsec / 1000 + rtime0str.tv_sec * 1000000;
        int rtime1 = rtime1str.tv_nsec / 1000 + rtime1str.tv_sec * 1000000;

        fprintf(f, "%f, %f, %f\n",
                (utime1 - utime0) / (float)(rtime1 - rtime0) * 100.f,
                (stime1 - stime0) / (float)(rtime1 - rtime0) * 100.f,
                (utime1 + stime1 - utime0 - stime0) / (float)(rtime1 - rtime0) * 100.f);

        fclose(f);
    }
    pipe->destroyInstance();
    utils::infoMsg("Pipe deleted");
    ctrl->destroyInstance();
    utils::infoMsg("Controller deleted");
    
    return 0;
}
