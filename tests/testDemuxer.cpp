#include "../src/modules/headDemuxer/HeadDemuxerLibav.hh"
#include "../src/modules/transmitter/SinkManager.hh"
#include "../src/Controller.hh"
#include "../src/Utils.hh"
#include "../src/Jzon.h"

#include <csignal>
#include <vector>
#include <string>

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    run = false;
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

    std::vector<int> ids;
    std::vector<int> readers;
    if (vWId >= 0) {
        if (!pipe->createPath(vWId, receiverID, transmitterID, vWId, 1, ids)) {
            utils::errorMsg("Could not create Video path");
        } else {
	        pipe->connectPath(vWId);
        	readers.push_back(1);
		}
    }

    if (aWId >= 0) {
        if (!pipe->createPath(aWId, receiverID, transmitterID, aWId, 2, ids)) {
            utils::errorMsg("Could not create Audio path");
        } else {
	        pipe->connectPath(aWId);
    	    readers.push_back(2);
		}
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
