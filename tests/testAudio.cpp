#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/modules/liveMediaOutput/SinkManager.hh"
#include "../src/modules/videoResampler/VideoResampler.hh"
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#include "../src/AudioFrame.hh"
#include "../src/Controller.hh"
#include "../src/Utils.hh"

#include <csignal>
#include <vector>
#include <liveMedia/liveMedia.hh>
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

bool run = true;

void signalHandler( int signum )
{
    utils::infoMsg("Interruption signal received");
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->stop();
    run = false;
    
    utils::infoMsg("Workers Stopped");
}

void createOutputPathsAndSessions(int decId) 
{
    int mixMasterId = rand();
    int mixSlaveId = rand();

    int enc1Id = rand();
    int enc2Id = rand();

    int mixMasterWorkerId = rand();
    int mixSlaveWorkerId = rand();
    int encoderWorkerId1 = rand();
    int encoderWorkerId2 = rand();
    
    int pathToTx1Id = rand();
    int pathToTx2Id = rand();
    int pathToMixMasterId = rand();
    int pathToMixSlaveId = rand();

    std::vector<int> idsTx1({enc1Id});
    std::vector<int> idsTx2({enc2Id});

    std::vector<int> readersTx1;
    std::vector<int> readersTx2;

    std::string sessionId;

    AudioMixer *mixMaster;
    AudioMixer *mixSlave;

    AudioEncoderLibav *encoder1;
    AudioEncoderLibav *encoder2;
    
    Master *mixerMasterWorker;
    Slave *mixerSlaveWorker;
    Master *encoderWorker1;
    Master *encoderWorker2;

    Path* pathToTx1;
    Path* pathToTx2;
    Path* pathToMixMaster;
    Path* pathToMixSlave;

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SinkManager *transmitter = pipe->getTransmitter();

    //NOTE: Adding mixers to pipeManager and handle worker
    mixMaster = new AudioMixer();
    mixSlave = new AudioMixer();
    pipe->addFilter(mixMasterId, mixMaster);
    pipe->addFilter(mixSlaveId, mixSlave);
    mixerMasterWorker = new Master();
    mixerSlaveWorker = new Slave();
    mixerMasterWorker->addProcessor(mixMasterId, mixMaster);
    mixerSlaveWorker->addProcessor(mixSlaveId, mixSlave);
    mixMaster->setWorkerId(mixMasterWorkerId);
    mixSlave->setWorkerId(mixSlaveWorkerId);
    pipe->addWorker(mixMasterWorkerId, mixerMasterWorker);
    pipe->addWorker(mixSlaveWorkerId, mixerSlaveWorker);

    mixerMasterWorker->addSlave(mixSlaveWorkerId, mixerSlaveWorker);

    //NOTE: Adding encoders to pipeManager and handle worker
    encoder1 = new AudioEncoderLibav();
    encoder2 = new AudioEncoderLibav();
    pipe->addFilter(enc1Id, encoder1);
    pipe->addFilter(enc2Id, encoder2);
    encoderWorker1 = new Master();
    encoderWorker2 = new Master();
    encoderWorker1->addProcessor(enc1Id, encoder1);
    encoderWorker2->addProcessor(enc2Id, encoder2);
    encoder1->setWorkerId(encoderWorkerId1);
    encoder2->setWorkerId(encoderWorkerId2);
    pipe->addWorker(encoderWorkerId1, encoderWorker1);
    pipe->addWorker(encoderWorkerId2, encoderWorker2);

    //NOTE: create, add and connect paths
    std::vector<int> dummy;
    pathToMixMaster = pipe->createPath(decId, mixMasterId, -1, -1, dummy);
    pipe->addPath(pathToMixMasterId, pathToMixMaster);       
    pipe->connectPath(pathToMixMaster);
    
    pathToMixSlave = pipe->createPath(decId, mixSlaveId, -1, -1, dummy, true);
    pipe->addPath(pathToMixSlaveId, pathToMixSlave);       
    pipe->connectPath(pathToMixSlave);

    pathToTx1 = pipe->createPath(mixMasterId, pipe->getTransmitterID(), -1, -1, idsTx1);
    pipe->addPath(pathToTx1Id, pathToTx1);       
    pipe->connectPath(pathToTx1);

    pathToTx2 = pipe->createPath(mixSlaveId, pipe->getTransmitterID(), -1, -1, idsTx2);
    pipe->addPath(pathToTx2Id, pathToTx2);       
    pipe->connectPath(pathToTx2);

    readersTx1.push_back(pathToTx1->getDstReaderID());
    readersTx2.push_back(pathToTx2->getDstReaderID());

    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (!transmitter->addSession(sessionId, readersTx1)){
        return;
    }
    transmitter->publishSession(sessionId);

    sessionId = utils::randomIdGenerator(ID_LENGTH);
    if (!transmitter->addSession(sessionId, readersTx2)){
        return;
    }
    transmitter->publishSession(sessionId);
        
    pipe->startWorkers();
}

int addAudioSource(unsigned port, std::string codec = A_CODEC, 
                    unsigned channels = A_CHANNELS, unsigned freq = A_TIME_STMP_FREQ)
{
    int aDecId = rand();
    int decId = rand();
    std::vector<int> ids;
    std::string sessionId;
    std::string sdp;
    
    AudioDecoderLibav *decoder;
    
    Master* aDec;
    
    Session *session;
    Path *path;
    
    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    SourceManager *receiver = pipe->getReceiver();
       
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    sdp = SourceManager::makeSessionSDP(sessionId, "this is an audio stream");    
    sdp += SourceManager::makeSubsessionSDP(A_MEDIUM, PROTOCOL, A_PAYLOAD, codec, 
                                            A_BANDWITH, freq, port, channels);
    utils::infoMsg(sdp);
    
    session = Session::createNew(*(receiver->envir()), sdp, sessionId);
    if (!receiver->addSession(session)){
        utils::errorMsg("Could not add audio session");
        return -1;
    }
    if (!session->initiateSession()){
        utils::errorMsg("Could not initiate audio session");
        return -1;
    }
    
    //NOTE: Adding decoder to pipeManager and handle worker
    decoder = new AudioDecoderLibav();
    pipe->addFilter(decId, decoder);
    aDec = new Master();
    aDec->addProcessor(decId, decoder);
    decoder->setWorkerId(aDecId);
    pipe->addWorker(aDecId, aDec);
    
    //NOTE: add filter to path
    path = pipe->createPath(pipe->getReceiverID(), decId, port, -1, ids);
    pipe->addPath(port, path);       
    pipe->connectPath(path);
        
    pipe->startWorkers();

    return decId;
}

int main(int argc, char* argv[]) 
{   
    int port;
    int decoder;
    utils::setLogLevel(INFO);

    if (argc < 2) {
        utils::errorMsg("Usage: testaudio <port>");
        return 1;
    }

    port = std::stoi(argv[1]);
    utils::infoMsg("Audio input port added: " + std::to_string(port));

    PipelineManager *pipe = Controller::getInstance()->pipelineManager();
    pipe->start();

    signal(SIGINT, signalHandler);

    decoder = addAudioSource(port);
    createOutputPathsAndSessions(decoder); 
       
    while (run) {
        sleep(1);
    }
 
    return 0;
}

