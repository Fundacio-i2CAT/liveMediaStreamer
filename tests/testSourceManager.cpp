#include "../src/network/SourceManager.hh"
#include "../src/network/ExtendedMediaSession.hh"
#include <liveMedia/liveMedia.hh>


#define V_MEDIUM "video"
#define PROTOCOL "RTP"
#define PAYLOAD 96
#define V_CODEC "H264"
#define BANDWITH 5000
#define V_CLIENT_PORT 6004
#define V_TIME_STMP_FREQ 90000

#define A_CODEC "AC3"
#define A_CLIENT_PORT 6006
#define A_MEDIUM "audio"
#define A_TIME_STMP_FREQ 44100



void usage(UsageEnvironment& env, char const* progName) {
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

int main(int argc, char** argv) 
{   
    char* sessionId;
    SourceManager *mngr = SourceManager::getInstance();
    
    if (argc < 2) {
        usage(*(mngr->envir()), argv[0]);
        return 1;
    }

    for (int i = 1; i <= argc-1; ++i) {
        char* id = (char *)malloc((ID_LENGTH+1)*sizeof(char));
        SourceManager::randomIdGenerator(id, ID_LENGTH);
        if (! mngr->addRTSPSession(id, argv[0], argv[i])) {
            *(mngr->envir()) << "\nFailed to create new session: " << id << "\n";
        }
    }
    
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
       
    mngr->runManager();
    
    sleep(5);
    
    mngr->initiateAll();
    
    sleep(20);
    
    return 0;
}







