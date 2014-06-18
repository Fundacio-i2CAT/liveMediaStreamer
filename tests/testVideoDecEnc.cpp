#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#include "../src/modules/liveMediaInput/SourceManager.hh"
#include "../src/Utils.hh"

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

#include <string>
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

void connect(char const* medium, unsigned short port){}

int main(int argc, char** argv) 
{   
    std::string sessionId;
    std::string sdp;
    Session* session;
    SourceManager *mngr = SourceManager::getInstance();
    FrameQueue* queue;
   	//Frame* rawFrame = InterleavedVideoFrame::createNew(RAW, DEFAULT_WIDTH, DEFAULT_HEIGHT, RGB24);
	Frame* h264Frame1080 = InterleavedVideoFrame::createNew(H264, 1920, 1080, YUYV422);
	Frame* h264Frame720 = InterleavedVideoFrame::createNew(H264, 1280, 720, YUYV422);
	Frame* h264Frame480 = InterleavedVideoFrame::createNew(H264, 720, 480, YUYV422);
    VideoDecoderLibav* decoder = new VideoDecoderLibav();
	VideoEncoderX264* encoder = new VideoEncoderX264();
	VideoEncoderX264* encoder720 = new VideoEncoderX264();
	VideoEncoderX264* encoder480 = new VideoEncoderX264();
	VideoEncoderX264* encoder1080 = new VideoEncoderX264();
	encoder720->configure(1280, 720, YUYV422);
	//encoder480->configure(720, 480, YUYV422);
	encoder1080->configure(1920, 1080, YUYV422);
    Worker *vDecoderWorker;
	Worker *vEncoderWorker;
	Master *vEnconderMaster;
	Slave *vEncoderSlave1, *vEncoderSlave2, *vEncoderSlave3;
    std::ofstream h264Frames720, h264Frames480, h264Frames1080;

	 mngr->setCallback((void(*)(char const*, unsigned short))&connect);
    
    //condif decoder
    
    signal(SIGINT, signalHandler); 
    
    for (int i = 1; i <= argc-1; ++i) {
        sessionId = utils::randomIdGenerator(ID_LENGTH);
        session = Session::createNewByURL(*(mngr->envir()), argv[0], argv[i], sessionId);
        mngr->addSession(session);
    }
    
    sessionId = utils::randomIdGenerator(ID_LENGTH);
    
    sdp = SourceManager::makeSessionSDP(sessionId, "this is a test");
    
    sdp += SourceManager::makeSubsessionSDP(V_MEDIUM, PROTOCOL, PAYLOAD, V_CODEC, 
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
//	if(!decoder->connectOneToOne(encoder720)) {
	if(!decoder->connectOneToOne(encoder)) {
        std::cerr << "Error connecting video encoder" << std::endl;
    }

    Reader *reader = new Reader();
	encoder->connect(reader);
    Reader *reader720 = new Reader();
	encoder720->connect(reader720);
	//Reader *reader480 = new Reader();
	//encoder480->connect(reader480);
	Reader *reader1080 = new Reader();
	encoder1080->connect(reader1080);

    vDecoderWorker = new Worker(decoder);
	vEnconderMaster = new Master(encoder);
	//vEncoderWorker = new Worker(encoder);
	
	vEncoderSlave1 = new Slave(1, encoder720);
	vEnconderMaster->addSlave(vEncoderSlave1);
	//vEncoderSlave2 = new Slave(2, encoder480);
	//vEnconderMaster->addSlave(vEncoderSlave2);
	vEncoderSlave3 = new Slave(3, encoder1080);
	vEnconderMaster->addSlave(vEncoderSlave3);
	

    vDecoderWorker->start();
	//vEncoderWorker->start();
	vEnconderMaster->start();
	vEncoderSlave1->start();
	//vEncoderSlave2->start();
	vEncoderSlave3->start();
    
    while(mngr->isRunning()) {
		//printf("antes getFrame\n");
		h264Frame720 = reader720->getFrame();
		//h264Frame480 = reader480->getFrame();
		h264Frame1080 = reader1080->getFrame();
		
        if (!h264Frame720 || !h264Frame1080) {
            usleep(500);
            continue;
        }
		//printf("despues getFrame\n");
        if (! h264Frames720.is_open()){
            h264Frames720.open("frames720.h264", std::ios::out | std::ios::app | std::ios::binary);
        }

       /* if (! h264Frames480.is_open()){
            h264Frames480.open("frames480.h264", std::ios::out | std::ios::app | std::ios::binary);
        }
*/
        if (! h264Frames1080.is_open()){
            h264Frames1080.open("frames1080.h264", std::ios::out | std::ios::app | std::ios::binary);
        }
		
		if (h264Frame720->getLength() > 0) {
            h264Frames720.write(reinterpret_cast<const char*>(h264Frame720->getDataBuf()), h264Frame720->getLength());
            //printf("Filled buffer! Frame size: %d\n", h264Frame->getLength());
        }

/*		if (h264Frame480->getLength() > 0) {
            h264Frames480.write(reinterpret_cast<const char*>(h264Frame480->getDataBuf()), h264Frame480->getLength());
            //printf("Filled buffer! Frame size: %d\n", h264Frame->getLength());
        }*/
        
		if (h264Frame1080->getLength() > 0) {
            h264Frames1080.write(reinterpret_cast<const char*>(h264Frame1080->getDataBuf()), h264Frame1080->getLength());
            //printf("Filled buffer! Frame size: %d\n", h264Frame->getLength());
        }

        reader720->removeFrame();        
       // reader480->removeFrame();
		reader1080->removeFrame();
    }
    
    h264Frames720.close();
	//h264Frames480.close();
	h264Frames1080.close();
    
    return 0;
}

