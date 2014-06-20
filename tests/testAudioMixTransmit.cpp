#include <liveMedia/liveMedia.hh>
#include <string>

#include "../src/AudioFrame.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
#include "../src/modules/audioMixer/AudioMixer.hh"
#include "../src/Controller.hh"
#include "../src/Callbacks.hh"
#include "../src/Utils.hh"

#include <iostream>
#include <csignal>
#include <sstream>


#define PROTOCOL "RTP"
#define PAYLOAD1 97
#define PAYLOAD2 97
#define BANDWITH 5000

#define A_CODEC1 "opus"
#define A_CODEC2 "pcmu"
#define A_CLIENT_PORT1 6006
#define A_CLIENT_PORT2 6008
#define A_MEDIUM "audio"
#define A_TIME_STMP_FREQ1 48000
#define A_TIME_STMP_FREQ2 8000
#define A_CHANNELS 2

bool should_stop = false;

struct buffer {
    unsigned char* data;
    int data_len;
};

void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    Controller *ctrl = Controller::getInstance();
    SourceManager *receiver = ctrl->pipelineManager()->getReceiver();
    SinkManager *transmitter = ctrl->pipelineManager()->getTransmitter();
    
    should_stop = true;
    receiver->closeManager();
    transmitter->closeManager();
    
    std::cout << "Managers closed\n";
    exit(1);
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    Session* session;

    PipelineManager *pipeMngr = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipeMngr->getReceiver();
    SinkManager *transmitter = pipeMngr->getTransmitter();

    receiver->setCallback(callbacks::connectToMixerCallback);
    AudioMixer *mixer = new AudioMixer(4);

    Worker* audioMixerWorker = new Worker();

    int audioMixerID = rand();

    if(!pipeMngr->addFilter(audioMixerID, mixer)) {
        std::cerr << "Error adding mixer to the pipeline" << std::endl;
    }
    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = utils::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(receiver->envir()), argv[0], argv[i], sessionId);
        receiver->addSession(session);
    }
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    
    sdp = SourceManager::makeSessionSDP("testSession", "this is a test");
    
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD1, A_CODEC1, BANDWITH, 
                                        A_TIME_STMP_FREQ1, A_CLIENT_PORT1, A_CHANNELS);
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD2, A_CODEC2, BANDWITH, 
                                        A_TIME_STMP_FREQ2, A_CLIENT_PORT2, A_CHANNELS);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    
    receiver->addSession(session);

    session->initiateSession();
       
    receiver->runManager();
    transmitter->runManager();

    Path *path = new AudioEncoderPath(audioMixerID, DEFAULT_ID);
    dynamic_cast<AudioEncoderLibav*>(pipeMngr->getFilter(path->getFilters().front()))->configure(OPUS);

    path->setDestinationFilter(pipeMngr->getTransmitterID(), transmitter->generateReaderID());

    if (!pipeMngr->connectPath(path)) {
        //TODO: ERROR
        exit(1);
    }

    int encoderPathID = rand();

    if (!pipeMngr->addPath(encoderPathID, path)) {
        //TODO: ERROR
        exit(1);
    }

    sleep(2);
    
    std::vector<int> readers;
    readers.push_back(path->getDstReaderID());

    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (!transmitter->addSession(sessionId, readers)) {
        return 1;
    }

    transmitter->publishSession(sessionId);

    if(!pipeMngr->addWorker(audioMixerID, audioMixerWorker)) {
        std::cerr << "Error adding mixer worker" << std::endl;
    }

    audioMixerWorker->start();

    std::string command;
    std::string aux;
    int id;
    float volume;

    while(!should_stop) {
        std::cout << std::endl << "Please enter a command (changeVolume): ";
        std::cin >> command;

        if (command.compare("exit") == 0) {
            break;
        }

        std::cout << std::endl << "Please enter a stream ID: ";
        std::cin >> id;
        std::cout << std::endl << "Please enter a volume (0.0 to 1.0): ";
        std::cin >> volume;

        std::cout << std::endl << command << "  " << id << "  " << volume << " " << std::endl;

        Jzon::Object rootNode;
        Jzon::Object params;
        params.Add("id", (int)pipeMngr->getPath(id)->getDstReaderID());
        params.Add("volume", (float)volume);
        rootNode.Add("params", params);
        rootNode.Add("action", command);

        Event e(rootNode, std::chrono::system_clock::now(), 0);
        pipeMngr->getFilter(pipeMngr->getPath(id)->getDestinationFilterID())->pushEvent(e);
    }
    
    return 0;
}
