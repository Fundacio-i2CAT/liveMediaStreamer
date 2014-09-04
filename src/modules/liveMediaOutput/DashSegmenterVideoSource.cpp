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
	currentTime.tv_sec = 0;
	currentTime.tv_usec = 0;
	initialTime.tv_sec = 0;
	initialTime.tv_usec = 0;
	previousTime.tv_sec = 0;
	previousTime.tv_sec = 0;
	spsSize = 0;
	ppsSize = 0;
	durationSampleFloat = 0.00;
	decodeTimeFloat = 0.00;
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
	if ((currentTime.tv_sec != fPresentationTime.tv_sec) || (currentTime.tv_usec != fPresentationTime.tv_usec)) {
		previousTime = currentTime;
		currentTime = fPresentationTime;
	}

	if (fMaxSize < frame->getLength()){
		fFrameSize = fMaxSize;
		fNumTruncatedBytes = frame->getLength() - fMaxSize;
	} else {
		fNumTruncatedBytes = 0; 
		fFrameSize = frame->getLength();
	}

	if (initFile == false) {
		if (spsSize == 0) {
			initialTime = fPresentationTime;
			previousTime = fPresentationTime;
			spsSize = fFrameSize;
			sps = new unsigned char[spsSize];
			memcpy(sps, frame->getDataBuf(), spsSize);
		}
		else if (ppsSize == 0) {
			ppsSize = fFrameSize;
			pps = new unsigned char[ppsSize];
			memcpy(pps, frame->getDataBuf(), ppsSize);
			unsigned char *metadata, *metadata2, *metadata3, *destinationData;
			uint32_t metadataSize = 0, metadata2Size = 0, metadata3Size = 0, initVideo = 0;
		    	// METADATA
			metadata = new unsigned char[4];
			metadata[0] = 0x01;
			metadata[1] = 0x42;
			metadata[2] = 0xC0;
			metadata[3] = 0x1E;
			metadataSize = 4;

			// METADATA2
			metadata2 = new unsigned char[2];
			metadata2[0] = 0xFF;
			metadata2[1] = 0xE1;
			metadata2Size = 2;

			// METADATA3
			metadata3 = new unsigned char[1];
			metadata3[0] = 0x01;
			metadata3Size = 1;

			// DESTINATION DATA
			destinationData = new unsigned char [MAX_DAT];
			initVideo = init_video_handler(metadata, metadataSize, metadata2, metadata2Size, sps, &spsSize, metadata3, metadata3Size, pps, ppsSize, destinationData, &avContext);
			printf("Se genero INIT FILE: %u\n", initVideo);
			initFile = true;
			previousTime = fPresentationTime;
			memcpy(fTo, destinationData, initVideo);    			
		}
	}
	else {
		uint8_t isIntra = 0;
		uint32_t decodeT = decodeTime(previousTime, initialTime);
		uint32_t segmentD = segmentDuration(currentTime, previousTime);
		printf("Se agrega segmento con decodeTime: %u y segmentTime: %u\n", decodeT, segmentD);
		unsigned char* destinationData = new unsigned char [MAX_DAT];
		unsigned char* buff = frame->getDataBuf();
		if (((buff[0] & INTRA_MASK) == IDR_NAL) || ((buff[0] & INTRA_MASK) == SEI_NAL)) {
			isIntra = 1;
			utils::debugMsg ("Es intra!");
		}
		uint32_t videoSample = add_sample(frame->getDataBuf(), fFrameSize, segmentD, decodeT, VIDEO_TYPE, destinationData, isIntra, &avContext);
		if (videoSample <= I2ERROR_MAX) {
			delete destinationData;
		}
		else {
			memcpy(fTo, destinationData, videoSample);
		}
	}

	fReader->removeFrame();
	utils::debugMsg ("next frame!");
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

uint32_t DashSegmenterVideoSource::decodeTime(struct timeval a, struct timeval b) {
	uint32_t totalA = a.tv_sec * 1000000 + a.tv_usec;
	uint32_t totalB = b.tv_sec * 1000000 + b.tv_usec;
	uint32_t decodeTime = (totalA - totalB)/H264_FREQUENCY;
	decodeTimeFloat= ((float)(totalA - totalB))/((float)(H264_FREQUENCY));
	decodeTimeFloat-= decodeTime;
	if (decodeTimeFloat >= 0.5) 
		decodeTime++;
	return decodeTime;
}
uint32_t DashSegmenterVideoSource::segmentDuration(struct timeval a, struct timeval b) {
	uint32_t totalA = a.tv_sec * 1000000 + a.tv_usec;
	uint32_t totalB = b.tv_sec * 1000000 + b.tv_usec;
	uint32_t durationTime = (totalA - totalB)/H264_FREQUENCY;
	durationSampleFloat= ((float)(totalA - totalB))/((float)(H264_FREQUENCY));
	durationSampleFloat-= durationTime;
	if (durationSampleFloat >= 1) {
		durationTime++;
		durationSampleFloat--;
	}
	return durationTime;
}

