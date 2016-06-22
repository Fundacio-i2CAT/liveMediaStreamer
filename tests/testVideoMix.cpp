#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#include "../src/modules/videoMixer/VideoMixer.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/receiver/SourceManager.hh"
#include "../src/modules/transmitter/SinkManager.hh"
#include "../src/Controller.hh"
#include "../src/Utils.hh"
#include <sys/resource.h>

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
#define DEFAULT_MIX_WIDTH 640
#define DEFAULT_MIX_HEIGHT 360
#define DEFAULT_MIX_CHANNELS 9
#define DEFAULT_MIX_COLS 3 // number of columns of the layout = sqrt(MIX_CHANNELS)
#define DEFAULT_OUTPUT_BITRATE 4000
#define DEFAULT_OUTPUT_PERIOD 40000 // microseconds

bool run = true;
int layer = 0;
int mix_width = DEFAULT_MIX_WIDTH;
int mix_height = DEFAULT_MIX_HEIGHT;
int mix_channels = DEFAULT_MIX_CHANNELS;
int mix_cols = DEFAULT_MIX_COLS;
int out_bitrate = DEFAULT_OUTPUT_BITRATE;
int out_period = DEFAULT_OUTPUT_PERIOD;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
}

bool setupMixer(int mixerId, int transmitterId)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    VideoMixer* mixer;
    VideoEncoderX264* encoder;
    VideoResampler* resampler;

    int encId = rand();
    int resId = rand();
    int pathId = rand();

    std::vector<int> ids = {resId, encId};

    mixer = VideoMixer::createNew(mix_channels, mix_width, mix_height, std::chrono::microseconds(out_period));
    mix_cols = ceil(sqrt(mix_channels));
    pipe->addFilter(mixerId, mixer);

    resampler = new VideoResampler();
    pipe->addFilter(resId, resampler);
    resampler->configure(0, 0, 0, YUV420P);

    encoder = new VideoEncoderX264();
    //bitrate, fps, gop, lookahead, bframes, threads, annexB, preset
    encoder->configure(out_bitrate, 1000000 / out_period, 25, 25, 4, 4, true, "superfast");
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

    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);

    resampler = new VideoResampler();
    resampler->configure(mix_width / mix_cols, mix_height / mix_cols, 0, RGB24);
    pipe->addFilter(resId, resampler);

    if (!pipe->createPath(port, receiverId, mixerId, port, port, ids)) {
        utils::errorMsg("Error creating video path");
        return;
    }

    if (!pipe->connectPath(port)) {
        utils::errorMsg("Error connecting path");
        pipe->removePath(port);
        return;
    }

    mixer = dynamic_cast<VideoMixer*>(pipe->getFilter(mixerId));

    if (!mixer) {
        utils::errorMsg("Error getting videoMixer from pipe");
        return;
    }

    mixer->configChannel(port, 1.f / mix_cols, 1.f / mix_cols,
            (layer % mix_cols) / (float)mix_cols, (layer / mix_cols) / (float)mix_cols,
            layer, true, 1.0);
    layer++;

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

    int transmitterID = 11;
    int receiverId = 20;
    int mixerId = 15;

    SinkManager* transmitter = NULL;
    PipelineManager *pipe;

    utils::setLogLevel(DEBUG);

    std::string out_address;
    int out_port = 0;
    int elapsed_time = 0, timeout = 0;

    std::string stats_filename;

    for (int i = 1; i < argc; i++) {

        if (strcmp(argv[i],"-viport") == 0) {
            int port = std::stoi(argv[i+1]);
            ports.push_back(port);
            utils::infoMsg("video input port: " + std::to_string(port));
        } else if (strcmp(argv[i],"-ri")==0) {
            std::string rtspUri = argv[i+1];
            uris.push_back(rtspUri);
            utils::infoMsg("input RTSP URI: " + rtspUri);
        } else if (strcmp(argv[i],"-ow")==0) {
            mix_width = std::stoi(argv[i+1]);
            utils::infoMsg("output width: " + std::to_string(mix_width));
        } else if (strcmp(argv[i],"-oh")==0) {
            mix_height = std::stoi(argv[i+1]);
            utils::infoMsg("output height: " + std::to_string(mix_height));
        } else if (strcmp(argv[i],"-ob")==0) {
            out_bitrate = std::stoi(argv[i+1]);
            utils::infoMsg("output bitrate: " + std::to_string(out_bitrate));
        } else if (strcmp(argv[i],"-op")==0) {
            out_period = std::stoi(argv[i+1]);
            utils::infoMsg("output period: " + std::to_string(out_period) + "us.");
        } else if (strcmp(argv[i],"-oaddr")==0) {
            out_address = argv[i+1];
            utils::infoMsg("output RTP address: " + out_address);
        } else if (strcmp(argv[i],"-oport")==0) {
            out_port = std::stoi(argv[i+1]);
            utils::infoMsg("output RTP port: " + std::to_string(out_port));
        } else if (strcmp(argv[i],"-statsfile")==0) {
            stats_filename = argv[i+1];
            utils::infoMsg("output stats filename: " + stats_filename);
        } else if (strcmp(argv[i],"-timeout")==0) {
            timeout = std::stoi(argv[i+1]);
            utils::infoMsg("timeout: " + std::to_string(timeout) + "s.");
        }
    }

    mix_channels = ports.size() + uris.size();

    if (ports.empty() && uris.empty()) {
        utils::errorMsg(
                "Usage: -viport <video input port> -ri <input RTSP uri>\n"
                "-ow <output width in pixels> -oh <output height in pixels>\n"
                "-ob <output bitrate> -op <output period in microseconds>\n"
                "-statsfile <output statistics filename>\n"
                "-timeout <seconds to wait before closing. 0 means forever>"
                "-oaddr <optional output RTP IP address> -oport <optional output RTP port>\n");
        return 1;
    }

    pipe = Controller::getInstance()->pipelineManager();

    transmitter = SinkManager::createNew();

    if (!transmitter) {
        utils::errorMsg("RTSPServer constructor failed");
        return 1;
    }

    pipe->addFilter(transmitterID, transmitter);

    setupMixer(mixerId, transmitterID);

    signal(SIGINT, signalHandler);

    for (auto it : pipe->getPaths()) {
        readers.push_back(it.second->getDstReaderID());
    }

    if (!publishRTSPSession(readers, transmitter)){
        utils::errorMsg("Failed adding RTSP sessions!");
        return 1;
    }

    if (!out_address.empty() && out_port != 0) {
        if (transmitter->addRTPConnection(readers, rand(), out_address, out_port, STD_RTP)) {
            utils::infoMsg("Added output RTP connection for " + out_address + ":" + std::to_string(out_port));
        }
    }

    for (auto p : ports) {
        SourceManager* receiver = new SourceManager();
        pipe->addFilter(receiverId, receiver);
        addVideoSDPSession(p, receiver);
        addVideoPath(p, receiverId, mixerId);
        receiverId++;
    }

    for (auto uri : uris) {
        SourceManager* receiver = new SourceManager();
        pipe->addFilter(receiverId, receiver);
        if (!addRTSPsession(uri, receiver, receiverId, mixerId)){
            utils::errorMsg("Couldn't start rtsp client session!");
            return 1;
        }
        receiverId++;
    }

    struct rusage usage0;
    getrusage(RUSAGE_SELF, &usage0);

    elapsed_time = 0;
    while(run) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (timeout) {
            elapsed_time++;
            if (elapsed_time == timeout) {
                run = false;
            }
        }
    }

    struct rusage usage1;
    getrusage(RUSAGE_SELF, &usage1);

    if (!stats_filename.empty()) {
        Jzon::Object pipe_state;
        pipe->getStateEvent(NULL, pipe_state);

        FILE *f = fopen(stats_filename.c_str(), "a+t");
        fprintf(f, "%d, %d, ", out_bitrate, mix_channels);

        const Jzon::Array &paths = pipe_state.Get("paths");
        int inDelay = 0, inLosses = 0, inCount = 0;
        int outDelay, outLosses;
        for (Jzon::Array::const_iterator it = paths.begin(); it != paths.end(); ++it) {
            int originFilter = (*it).Get("originFilter").ToInt();
            if (originFilter >= 20 && originFilter < 50) {
                // Input path: Receiver -> Mixer
                inDelay += (*it).Get("avgDelay").ToInt();
                inLosses += (*it).Get("lostBlocs").ToInt();
                inCount++;
            } else {
                // Output path: Mixer -> Transmitter
                outDelay = (*it).Get("avgDelay").ToInt();
                outLosses = (*it).Get("lostBlocs").ToInt();
            }
        }
        inDelay /= inCount;

        fprintf(f, "%d, %d, %d, ", inDelay, outDelay, inDelay + outDelay);
        fprintf(f, "%d, %d, %d, ", inLosses, outLosses, inLosses + outLosses);

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
        inPacketLoss /= inCount;

        fprintf(f, "%f, %f, ", inBitrate, inPacketLoss / inCount);

        Jzon::Object tran_state;
        transmitter->getState(tran_state);
        const Jzon::Array &sessions2 = tran_state.Get("sessions");
        float outBitrate = 0, outPacketLoss = 0;
        for (Jzon::Array::const_iterator it = sessions2.begin(); it != sessions2.end(); ++it) {
            if ((*it).Has("ip")) {
                // The RTP output session (the only one that runs when no
                // client is connected)
                const Jzon::Array &stats = (*it).Get("subsessionsStats").AsArray();
                for (Jzon::Array::const_iterator it2 = stats.begin(); it2 != stats.end(); ++it2) {
                    outBitrate = (*it2).Get("avgBitrateInKbps").ToInt();
                    outPacketLoss = (*it2).Get("packetLossRatio").ToInt();
                }
            }
        }
        fprintf(f, "%f, %f, ", outBitrate, outPacketLoss);

        int time0 = usage0.ru_utime.tv_usec + usage0.ru_utime.tv_sec * 1000000 +
                    usage0.ru_stime.tv_usec + usage0.ru_stime.tv_sec * 1000000;
        int time1 = usage1.ru_utime.tv_usec + usage1.ru_utime.tv_sec * 1000000 +
                    usage1.ru_stime.tv_usec + usage1.ru_stime.tv_sec * 1000000;

        fprintf(f, "%f\n", (time1 - time0) / 1e6 / (float)timeout * 100.f);

        fclose(f);
    }

    Controller::destroyInstance();
    PipelineManager::destroyInstance();

    return 0;
}
