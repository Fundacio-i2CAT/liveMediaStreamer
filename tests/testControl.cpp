#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#include <string>

#ifndef _HANDLERS_HH
#include "../src/network/Handlers.hh"
#endif

#ifndef _SOURCE_MANAGER_HH
#include "../src/network/SourceManager.hh"
#endif

#include "../src/AudioFrame.hh"
#include "../src/modules/audioDecoder/AudioDecoderLibav.hh"
#include "../src/modules/audioEncoder/AudioEncoderLibav.hh"
#include "../src/modules/audioMixer/AudioMixer.hh"
#include "../src/Controller.hh"
#include "../src/Callbacks.hh"

#include <iostream>
#include <csignal>
#include <sstream>


#define PROTOCOL "RTP"
#define PAYLOAD 97
#define BANDWITH 5000

#define A_CODEC "opus"
#define A_CLIENT_PORT1 6006
#define A_CLIENT_PORT2 6008
#define A_MEDIUM "audio"
#define A_TIME_STMP_FREQ 48000
#define A_CHANNELS 2

#define CHANNEL_MAX_SAMPLES 3000
#define OUT_SAMPLE_RATE 48000

bool should_stop = false;

struct buffer {
    unsigned char* data;
    int data_len;
};

void signalHandler( int signum )
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    SourceManager *mngr = SourceManager::getInstance();
    mngr->closeManager();
    
    std::cout << "Manager closed\n";
}

void fillBuffer(struct buffer *b, Frame *pFrame) {
    memcpy(b->data + b->data_len, pFrame->getDataBuf(), pFrame->getLength());
    b->data_len += pFrame->getLength(); 
}

void saveBuffer(struct buffer *b) 
{
    FILE *audioChannel = NULL;
    char filename[32];

    sprintf(filename, "coded.mp3");

    audioChannel = fopen(filename, "wb");

    if (b->data != NULL) {
        fwrite(b->data, b->data_len, 1, audioChannel);
    }

    fclose(audioChannel);
}

void readingRoutine(struct buffer* b, Reader *reader)
{
    Frame *codedFrame;
    Controller *ctrl = Controller::getInstance();
    SourceManager *mngr = ctrl->pipelineManager()->getReceiver();

    while(mngr->isRunning()) {
        codedFrame = reader->getFrame();

        if (!codedFrame) {
            usleep(500);
            continue;
        }

        fillBuffer(b, codedFrame);
        //printf("Filled buffer! Frame size: %d\n", codedFrame->getLength());

        reader->removeFrame();
    }
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    Session* session;

    PipelineManager *pipeMngr = Controller::getInstance()->pipelineManager();
    SourceManager *mngr = pipeMngr->getReceiver();
    mngr->setCallback(callbacks::connectToMixerCallback);
    AudioMixer *mixer = new AudioMixer(4);
    AudioEncoderLibav* audioEncoder = new AudioEncoderLibav();
    audioEncoder->configure(MP3);

    Worker* audioEncoderWorker = new Worker(audioEncoder);
    Worker* audioDecoder1Worker = new Worker();
    Worker* audioDecoder2Worker = new Worker();
    Worker* audioMixerWorker = new Worker();

    struct buffer *buffers;
    unsigned int bytesPerSample = 2;
    int audioMixerID = rand();

    if(!pipeMngr->addFilter(audioMixerID, mixer)) {
        std::cerr << "Error adding mixer to the pipeline" << std::endl;
    }
    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = handlers::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i], sessionId);
        mngr->addSession(session);
    }
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    
    sdp = handlers::makeSessionSDP("testSession", "this is a test");
    
    sdp += handlers::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD, A_CODEC, BANDWITH, 
                                        A_TIME_STMP_FREQ, A_CLIENT_PORT1, A_CHANNELS);
    sdp += handlers::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD, A_CODEC, BANDWITH, 
                                        A_TIME_STMP_FREQ, A_CLIENT_PORT2, A_CHANNELS);
    
    session = Session::createNew(*(mngr->envir()), sdp, sessionId);
    
    mngr->addSession(session);

    session->initiateSession();
       
    mngr->runManager();

    if(!mixer->connectOneToOne(audioEncoder)) {
        std::cerr << "Error connecting mixer with encoder" << std::endl;
    }

    buffers = new struct buffer;
    buffers->data = new unsigned char[CHANNEL_MAX_SAMPLES * bytesPerSample * OUT_SAMPLE_RATE * 360]();
    buffers->data_len = 0;

    if(!pipeMngr->addWorker(audioMixerID, audioMixerWorker)) {
        std::cerr << "Error adding mixer worker" << std::endl;
    }

    pipeMngr->addWorkerToPath(pipeMngr->getPath(A_CLIENT_PORT1), audioDecoder1Worker);
    pipeMngr->addWorkerToPath(pipeMngr->getPath(A_CLIENT_PORT2), audioDecoder2Worker);

    Reader *reader = new Reader();
    audioEncoder->connect(reader);

    audioDecoder1Worker->start();
    audioDecoder2Worker->start();
    audioMixerWorker->start();
    audioEncoderWorker->start();

    std::thread readingThread(readingRoutine, buffers, reader);

    std::string command;
    std::string aux;
    int id;
    float volume;

    while(true) {
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

        Event e(rootNode, std::chrono::system_clock::now());
        pipeMngr->getFilter(pipeMngr->getPath(id)->getDestinationFilterID())->pushEvent(e);
    }
    
    readingThread.join();
    saveBuffer(buffers);
    printf("Buffer saved\n");

    return 0;
}
