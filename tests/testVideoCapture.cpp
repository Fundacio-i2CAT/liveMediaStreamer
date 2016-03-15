#include "../src/Controller.hh"
#include "../src/Utils.hh"
#include "../src/Jzon.h"
#include "../src/modules/transmitter/SinkManager.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/modules/V4LCapture/V4LCapture.hh"

#include <csignal>
#include <vector>
#include <string>

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
}


bool publishRTSPSession(std::vector<int> readers, SinkManager *transmitter)
{
    std::string sessionId;
    std::vector<int> byPassReaders;

    bool ret = false;
    
    sessionId = "plainrtp";
    utils::infoMsg("Adding plain RTP session...");
    ret |= transmitter->addRTSPConnection(readers, 1, STD_RTP, sessionId);
    
    sessionId = "mpegts";
    utils::infoMsg("Adding plain MPEGTS session...");
    ret |= transmitter->addRTSPConnection(readers, 2, MPEGTS, sessionId);
    
    return ret;
}

int main(int argc, char* argv[])
{
    int cPort = 7777;
    std::vector<int> readers;
    std::vector<int> midFilters;
    std::string cam;
    int width = 0;
    int height = 0;
    unsigned fps = 0;

    PipelineManager *pipe;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-cam")==0) {
            cam = argv[i+1];
            utils::infoMsg("camera: " + cam);
        } else if (strcmp(argv[i],"-w")==0) {
            width = std::stoi(argv[i+1]);
            utils::infoMsg("desired width: " + std::to_string(width));
        } else if (strcmp(argv[i],"-h")==0) {
            height = std::stoi(argv[i + 1]);
            utils::infoMsg("desired width: " + std::to_string(height));
        } else if (strcmp(argv[i],"-r")==0) {
            fps = std::stoi(argv[i+1]);
            utils::infoMsg("frame rate: " + std::to_string(fps));
        } 
    }

    pipe = Controller::getInstance()->pipelineManager();
    
    SinkManager* transmitter = SinkManager::createNew();
    if (!transmitter){
        utils::errorMsg("RTSPServer constructor failed");
        return 1;
    }
    
    V4LCapture *capture = new V4LCapture();
    VideoResampler *resampler = new VideoResampler();
    VideoEncoderX264 *encoder = new VideoEncoderX264();
    
    if (!capture->configure(cam, width, height, fps, "YUYV", true)){
        return 1;
    }
    resampler->configure(0, 0, 0, YUV420P);
    encoder->configure(4000, 0, 25, 0, 0, 4, true, "superfast");
    
    int captureID = 1003;
    int resamplerID = 1001;
    int encoderID = 1000;
    int transmitterID = 1024;

    pipe->addFilter(captureID, capture);
    
    pipe->addFilter(resamplerID, resampler);
    midFilters.push_back(resamplerID);
    
    pipe->addFilter(encoderID, encoder);
    midFilters.push_back(encoderID);
    
    pipe->addFilter(transmitterID, transmitter); 
    
    if (!pipe->createPath(1, captureID, transmitterID, -1, -1, midFilters)) {
        utils::errorMsg("Error creating video path");
        return 1;
    }   

    if (!pipe->connectPath(1)) {
        utils::errorMsg("Failed! Path not connected");
        pipe->removePath(1);
        return 1;
    }

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

    Controller* ctrl = Controller::getInstance();

    if (!ctrl->createSocket(cPort)) {
        exit(1);
    }

    while (run) {
        if (!ctrl->listenSocket()) {
            continue;
        }

        if (!ctrl->readAndParse()) {
            continue;
        }

        ctrl->processRequest();
    }

    PipelineManager::destroyInstance();
    Controller::destroyInstance();
    
    return 0;
}


