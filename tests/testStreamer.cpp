#include "../src/network/SourceManager.hh"
#include "../src/network/SinkManager.hh"
#include "../src/network/ExtendedMediaSession.hh"
#include <liveMedia.hh>


#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define PAYLOAD 96
#define V_CODEC "H264"
#define BANDWITH 5000
#define V_CLIENT_PORT 6004
#define V_TIME_STMP_FREQ 90000

#define A_CODEC "AAC"
#define A_CLIENT_PORT 6006
#define A_MEDIUM "audio"
#define A_TIME_STMP_FREQ 44100



void usage(UsageEnvironment& env, char const* progName) {
  env << "Usage: " << progName << " NO ARGUMENTS!\n";
}

int main(int argc, char** argv) 
{   
    char* sessionId;
    bool run = true;
    
    SourceManager *mngr = SourceManager::getInstance();
    SinkManager sMngr = SinkManager::getInstance();
       
    mngr->runManager();
    sMngr->runManager();
    
    //Receiver
    
    sessionId = (char *)malloc((ID_LENGTH+1)*sizeof(char));
    SourceManager::randomIdGenerator(sessionId, ID_LENGTH);
    
    if (! mngr->addSession(sessionId, (char *) V_MEDIUM, (char *)"testSession", (char *)"this is a test")){
        *(mngr->envir()) << "\nFailed to create new session: " << sessionId << "\n";
    }
    
    Session *session = mngr->getSession(sessionId);
    
    ExtendedMediaSession *mSession = (ExtendedMediaSession *) session->session;
    
    
    ExtendedMediaSubsession *vSubsession = ExtendedMediaSubsession::createNew(*mSession);
     
    vSubsession->setMediaSubsession((char *) V_MEDIUM, (char *) PROTOCOL, 
                                   (unsigned char) PAYLOAD, (char *) V_CODEC, BANDWITH, 
                                   V_TIME_STMP_FREQ, V_CLIENT_PORT);
    
    ExtendedMediaSubsession *aSubsession = ExtendedMediaSubsession::createNew(*mSession);
    
    aSubsession->setMediaSubsession((char *) A_MEDIUM, (char *) PROTOCOL, 
                                   (unsigned char) PAYLOAD, (char *) A_CODEC, BANDWITH, 
                                   A_TIME_STMP_FREQ, A_CLIENT_PORT);
    
    mSession->addSubsession(vSubsession);
    mSession->addSubsession(aSubsession);
    
    mngr->initiateAll();
    
    //Transmitter
    char const* streamName = "h264_test";
    char const* description = "restreaming h264 data";
    
    SinkManager::randomIdGenerator(sessionId, ID_LENGTH);
    if (! sMngr->addSession(sessionId, streamName, streamName, description)){
        return -1;
    }        
        
    ServerMediaSession* sSession  = Mngr->getSession(sessionId); 
    if (sSession == NULL){
        return -1;
    }
    
    sSession->addSubsession(H264QueueServerMediaSession
               ::createNew(sMngr->envir(), queue, False));
    
    
    while(run){
        usleep(1000);
    }
    
    return 0;
}