#include "DashSegmenterVideoSource.hh"
#include "SinkManager.hh"
#include <iostream>


DashSegmenterVideoSource* DashSegmenterVideoSource::createNew(UsageEnvironment& env, Reader *reader, int readerId, uint32_t frameRate)
{
	return new DashSegmenterVideoSource(env, reader, readerId, frameRate);
}

DashSegmenterVideoSource::DashSegmenterVideoSource(UsageEnvironment& env, Reader *reader, int readerId, uint32_t frameRate): FramedSource(env), fReader(reader), fReaderId(readerId), fInputSource(NULL)
{
	uint8_t i2error;
	i2error= context_initializer(&avContext, VIDEO_TYPE);
	if (i2error == I2ERROR_MEDIA_TYPE) {
		utils::errorMsg ("Media type incorrect");
	}
	initFile = false;
	presentationTime.tv_sec = 0;
	presentationTime.tv_usec = 0;
	previousTime.tv_sec = 0;
	previousTime.tv_sec = 0;
	spsSize = 0;
	ppsSize = 0;
}

DashSegmenterVideoSource::~DashSegmenterVideoSource() {
	doStopGettingFrames();
	delete avContext;
}

void DashSegmenterVideoSource::doGetNextFrame(){
	if ((frame = fReader->getFrame()) == NULL) {
		nextTask() = envir().taskScheduler().scheduleDelayedTask(POLL_TIME,
		(TaskFunc*)DashSegmenterVideoSource::staticDoGetNextFrame, this);
	return;
	}

	fPresentationTime.tv_sec = frame->getPresentationTime().count()/1000000;
	fPresentationTime.tv_usec = frame->getPresentationTime().count()%1000000;
	presentationTime = fPresentationTime;

	if (fMaxSize < frame->getLength()){
		fFrameSize = fMaxSize;
		fNumTruncatedBytes = frame->getLength() - fMaxSize;
	} else {
		fNumTruncatedBytes = 0; 
		fFrameSize = frame->getLength();
	}

	if (initFile == false) {
		if (spsSize == 0) {
			spsSize = fFrameSize;
			sps = new unsigned char[spsSize];
			memcpy(sps, frame->getDataBuf(), spsSize);
		}
		else if (ppsSize == 0) {
			ppsSize = fFrameSize;
			pps = new unsigned char[ppsSize];
			memcpy(pps, frame->getDataBuf(), ppsSize);
			unsigned char *metadata, *metadata2, *metadata3, *destination_data;
			uint32_t metadata_size = 0, metadata2_size = 0, metadata3_size = 0, init_video = 0;
		    	// METADATA
			metadata = new unsigned char[4];
			metadata[0] = 0x01;
			metadata[1] = 0x42;
			metadata[2] = 0xC0;
			metadata[3] = 0x1E;
			metadata_size = 4;

			// METADATA2
			metadata2 = new unsigned char[2];
			metadata2[0] = 0xFF;
			metadata2[1] = 0xE1;
			metadata2_size = 2;

			// METADATA3
			metadata3 = new unsigned char[1];
			metadata3[0] = 0x01;
			metadata3_size = 1;

			// DESTINATION DATA
			destination_data = (byte *) malloc (MAX_DAT);
			init_video = init_video_handler(metadata, metadata_size, metadata2, metadata2_size, sps, &spsSize, metadata3, metadata3_size, pps, ppsSize, destination_data, &avContext);
			printf("Se genero INIT FILE: %u\n", init_video);
			initFile = true;
			previousTime = fPresentationTime;
			memcpy(fTo, destination_data, init_video);    			
		}
	}
	else {
		//uint32_t video_sample;
		//video_sample = add_sample(frame->getDataBuf(), fFrameSize, uint32_t duration_sample, uint32_t timestamp, uint32_t media_type, byte *output_data, uint8_t is_intra, i2ctx **context);
	}

	fReader->removeFrame();
	//utils::debugMsg ("NADA");
	afterGetting(this);
}

void DashSegmenterVideoSource::checkStatus()
{
    if (fReader->isConnected()) {
        return;
    }

    stopGettingFrames();
}

void DashSegmenterVideoSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}

