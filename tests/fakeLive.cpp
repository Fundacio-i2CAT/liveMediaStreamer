#include <liveMedia.hh>
//#include <BasicUsageEnvironment.hh>
#include "../src/modules/CustomScheduler.hh"
#include <string>

#include "H264LoopVideoFileServerMediaSubsession.hh"

UsageEnvironment* env;

// To make the second and subsequent client for each stream reuse the same
// input stream as the first client (rather than playing the file from the
// start for each client), change the following "False" to "True":
Boolean reuseFirstSource = True;

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
                           char const* streamName, char const* inputFileName); // fwd

int main(int argc, char** argv) {
  // Begin by setting up our usage environment:
  TaskScheduler* scheduler = CustomScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
  // To implement client access control to the RTSP server, do the following:
  authDB = new UserAuthenticationDatabase;
  authDB->addUserRecord("username1", "password1"); // replace these with real strings
  // Repeat the above with each <username>, <password> that you wish to allow
  // access to the server.
#endif

  OutPacketBuffer::maxSize = 1000000;
  
    std::string vStreamName = "";
    std::string aStreamName = "";
    RTSPServer* rtspServer = NULL;
    
    unsigned port = 8666;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i],"-v")==0) {
            vStreamName = argv[i+1];
            *env << "video input file: " << vStreamName.c_str() << "\n";
        } else if (strcmp(argv[i],"-a")==0) {
            aStreamName = argv[i+1];
            *env << "audio input file: " << aStreamName.c_str() << "\n";
        }
    }
    
    if (vStreamName.length() == 0 && aStreamName.length() == 0){
        return 1;
    }
  
  // Create the RTSP server:
  while (!rtspServer){
      rtspServer = RTSPServer::createNew(*env, port, authDB);
      port += 2;
      if (port >= 9000){
          *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
            exit(1);
      }
  }

  char const* descriptionString
    = "Session streamed by \"testOnDemandRTSPServer\"";

  // A H.264 video elementary stream:
  { 
    ServerMediaSession* sms
      = ServerMediaSession::createNew(*env, vStreamName.c_str(), vStreamName.c_str(),
                                      descriptionString);
      
    Boolean useADUs = False;
    Interleaving* interleaving = NULL;
#ifdef STREAM_USING_ADUS
    useADUs = True;
#ifdef INTERLEAVE_ADUS
    unsigned char interleaveCycle[] = {0,2,1,3}; // or choose your own...
    unsigned const interleaveCycleSize
      = (sizeof interleaveCycle)/(sizeof (unsigned char));
    interleaving = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
    if (aStreamName.length() > 0){
        sms->addSubsession(MP3AudioFileServerMediaSubsession
                       ::createNew(*env, aStreamName.c_str(), reuseFirstSource,
                                   useADUs, interleaving));
    }
    if (vStreamName.length() > 0){
        sms->addSubsession(H264LoopVideoFileServerMediaSubsession
                       ::createNew(*env, vStreamName.c_str(), reuseFirstSource));
    }
    rtspServer->addServerMediaSession(sms);

    announceStream(rtspServer, sms, vStreamName.c_str(), vStreamName.c_str());
  }

  while(true){ // does not return
    env->taskScheduler().doEventLoop(); 
  }

  return 0; // only to prevent compiler warning
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
                           char const* streamName, char const* inputFileName) {
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  env << "\n\"" << streamName << "\" stream, from the file \""
      << inputFileName << "\"\n";
  env << "Play this stream using the URL \"" << url << "\"\n";
  delete[] url;
}
