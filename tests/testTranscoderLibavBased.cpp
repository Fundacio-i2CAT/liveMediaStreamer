#include "../src/modules/headDemuxer/HeadDemuxerLibav.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/Controller.hh"
#include "../src/Utils.hh"
#include "../src/Jzon.h"

#include <csignal>
#include <vector>
#include <string>

#define OUT_A_CODEC AAC
#define OUT_A_FREQ 48000
#define OUT_A_BITRATE 192000
#define OUT_A_CHANNELS 2

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
}

bool addVideoPath(int vWId, int receiverID, int transmitterID)
{
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();

    int decId = 500;
    int resId = 2000;
    int encId = 1000;

    std::vector<int> ids;

    ids = {decId, resId, encId};

    VideoResampler *resampler;
    VideoEncoderX264 *encoder;
    VideoDecoderLibav *decoder;

    decoder = new VideoDecoderLibav();
    pipe->addFilter(decId, decoder);

    resampler = new VideoResampler();
    pipe->addFilter(resId, resampler);
    resampler->configure(1920, 1080, 0, YUV420P);

    encoder = new VideoEncoderX264();
    pipe->addFilter(encId, encoder);

    //bitrate, fps, gop, lookahead, threads, annexB, preset
    encoder->configure(4000, 25, 25, 0, 4, true, "superfast");
       
    if (!pipe->createPath(vWId, receiverID, transmitterID, vWId, -1, ids)) {
        utils::errorMsg("Error creating video path");
        return false;
    }   

    if (!pipe->connectPath(vWId)) {
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(vWId);
        return false;
    }

    utils::infoMsg("Video path created from wId " + std::to_string(vWId));
    return true;
}

bool addAudioPath(unsigned port, int receiverID, int transmitterID)
{
    PipelineManager *pipe = PipelineManager::getInstance();

    int decId = 101;
    int encId = 102;
    std::vector<int> ids({decId, encId});

    AudioDecoderLibav *decoder;
    AudioEncoderLibav *encoder;

    decoder = new AudioDecoderLibav();
    pipe->addFilter(decId, decoder);

    encoder = new AudioEncoderLibav();
    if (!encoder->configure(OUT_A_CODEC, OUT_A_CHANNELS, OUT_A_FREQ, OUT_A_BITRATE)) {
        utils::errorMsg("Error configuring audio encoder. Check provided parameters");
        return false;
    }

    pipe->addFilter(encId, encoder);
    
    if (!pipe->createPath(port, receiverID, transmitterID, port, -1, ids)) {
        utils::errorMsg("Error creating audio path");
        return false;
    }
    
    if (!pipe->connectPath(port)){
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(port);
        return false;
    }

    utils::infoMsg("Audio path created from port " + std::to_string(port));
    return true;
}

int main(int argc, char* argv[])
{
    int vWId = -1;
    int aWId = -1;
    int port = 0;
    std::string ip;
    std::string inUri;

    int transmitterID = 1023;
    int receiverID = 1024;

    SinkManager* transmitter = NULL;
    HeadDemuxerLibav* receiver = NULL;
    PipelineManager *pipe;

    utils::setLogLevel(INFO);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-d")==0) {
            ip = argv[i + 1];
            utils::infoMsg("destination IP: " + ip);
        } else if (strcmp(argv[i],"-P")==0) {
            port = std::stoi(argv[i+1]);
            utils::infoMsg("destination port: " + std::to_string(port));
        } else if (strcmp(argv[i],"-r")==0) {
            inUri = argv[i+1];
            utils::infoMsg("input URI: " + inUri);
        }
    }

    if (inUri.length() == 0){
        utils::errorMsg("I need at least an input uri (-r)");
        return 1;
    }

    receiver = new HeadDemuxerLibav();
    utils::infoMsg("Contacting URI...");
    receiver->setURI(inUri);

    pipe = Controller::getInstance()->pipelineManager();

    transmitter = SinkManager::createNew();
    if (!transmitter){
        utils::errorMsg("RTSPServer constructor failed");
        return 1;
    }

    pipe->addFilter(transmitterID, transmitter);

    pipe->addFilter(receiverID, receiver);

    signal(SIGINT, signalHandler);

    Jzon::Object state;
    receiver->getState(state);
    const Jzon::Array &streams = state.Get("streams");
    int count = streams.GetCount();
    utils::infoMsg("Found " + std::to_string(count) + " streams");
        
    // Chose an audio and a video stream at random (the last ones)
    for (Jzon::Array::const_iterator it = streams.begin(); it != streams.end(); ++it) {
        int type = (*it).Get("type").ToInt();
        int wId = (*it).Get("wId").ToInt();
        switch (type) {
        case VIDEO:
            vWId = wId;
            utils::infoMsg("  Selected Video wId " + std::to_string(vWId));
            break;
        case AUDIO:
            aWId = wId;
            utils::infoMsg("  Selected Audio wId " + std::to_string(aWId));
            break;
        default:
            break;
        }
    }

    std::vector<int> readers;
    if (vWId >= 0 && addVideoPath(vWId, receiverID, transmitterID)) {
        readers.push_back(pipe->getPath(vWId)->getDstReaderID());
    }

    if (aWId >= 0 && addAudioPath(aWId, receiverID, transmitterID)) {
        readers.push_back(pipe->getPath(aWId)->getDstReaderID());
    }

    if (!transmitter->addRTSPConnection(readers, 1, MPEGTS, "mpegts")){
        utils::errorMsg("Could not add RTSP output connection for MPEGTS");
    }
    if (!transmitter->addRTSPConnection(readers, 2, STD_RTP, "rtp")){
        utils::errorMsg("Could not add RTSP output connection RTP");
    }
    if (port != 0 && !ip.empty()) {
        if (transmitter->addRTPConnection(readers, 3, ip, port, MPEGTS)) {
            utils::infoMsg("Added connection for " + ip + ":" + std::to_string(port));
        }
    }

    while (run) {
        pause();
    }
    
    Controller::getInstance()->pipelineManager()->stop();
    Controller::destroyInstance();

    return 0;
}

