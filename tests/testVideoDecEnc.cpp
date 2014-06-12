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

#ifndef _VIDEO_DECODER_LIBAV_HH
#include "../src/modules/videoDecoder/VideoDecoderLibav.hh"
#endif

#ifndef _FRAME_QUEUE_HH
#include "../src/FrameQueue.hh"
#endif

#ifndef _INTERLEAVED_VIDEO_FRAME_HH
#include "../src/VideoFrame.hh"
#endif

#ifndef _VIDEO_ENCODER_X264_HH
#include "../src/modules/videoEncoder/VideoEncoderX264.hh"
#endif

#include <fstream>
#include <iostream>
#include <csignal>

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


void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    
    SourceManager *mngr = SourceManager::getInstance();
    mngr->closeManager();
    
    std::cout << "Manager closed\n";
}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    Session* session;
    SourceManager *mngr = SourceManager::getInstance();
    FrameQueue* queue;
   	//Frame* rawFrame = InterleavedVideoFrame::createNew(RAW, DEFAULT_WIDTH, DEFAULT_HEIGHT, RGB24);
	Frame* h264Frame = InterleavedVideoFrame::createNew(H264, DEFAULT_WIDTH, DEFAULT_HEIGHT, YUYV422);
    VideoDecoderLibav* decoder = new VideoDecoderLibav();
	VideoEncoderX264* encoder = new VideoEncoderX264();
    Worker *vDecoderWorker;
	Worker *vEncoderWorker;
    std::ofstream h264Frames;
    
    //condif decoder
    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = handlers::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i], sessionId);
        mngr->addSession(session);
    }
    
    sessionId = handlers::randomIdGenerator(ID_LENGTH);
    
    sdp = handlers::makeSessionSDP(sessionId, "this is a test");
    
    sdp += handlers::makeSubsessionSDP(V_MEDIUM, PROTOCOL, PAYLOAD, V_CODEC, 
                                       BANDWITH, V_TIME_STMP_FREQ, V_CLIENT_PORT);
    // sdp += handlers::makeSubsessionSDP(A_MEDIUM, PROTOCOL, PAYLOAD, A_CODEC, 
    //                                    BANDWITH, A_TIME_STMP_FREQ, A_CLIENT_PORT);
    
    session = Session::createNew(*(mngr->envir()), sdp, sessionId);
    
    mngr->addSession(session);
    session->initiateSession();
    
    mngr->runManager();
       
    //Let some time to initiate reciver sessions
    sleep(2);

    int id1 = V_CLIENT_PORT; //mngr->getWriterID(V_CLIENT_PORT);

    //if(!mngr->connect(id1, decoder, decoder->getAvailableReaders().front())) {
	if(!mngr->connectManyToOne(decoder, id1)) {
        std::cerr << "Error connecting video decoder" << std::endl;
    }
	
	//int id2 = decoder->getAvailableWriters().front();
	//if(!decoder->connect(id2, encoder, encoder->getAvailableReaders().front())) {
	if(!decoder->connectOneToOne(encoder)) {
        std::cerr << "Error connecting video encoder" << std::endl;
    }

    Reader *reader = new Reader();
    //encoder->connect(encoder->getAvailableWriters().front(), reader);
	encoder->connect(reader);

    vDecoderWorker = new Worker(decoder);
	vEncoderWorker = new Worker(encoder);

    vDecoderWorker->start(); 
	vEncoderWorker->start();
    
    while(mngr->isRunning()) {
		h264Frame = reader->getFrame();

        if (!h264Frame) {
            usleep(500);
            continue;
        }

        if (! h264Frames.is_open()){
            h264Frames.open("frames.h264", std::ios::out | std::ios::app | std::ios::binary);
        }
		
		if (h264Frame->getLength() > 0) {
            h264Frames.write(reinterpret_cast<const char*>(h264Frame->getDataBuf()), h264Frame->getLength());
            //printf("Filled buffer! Frame size: %d\n", h264Frame->getLength());
        }
        
        reader->removeFrame();
    }
    
    h264Frames.close();
    
    return 0;
}

