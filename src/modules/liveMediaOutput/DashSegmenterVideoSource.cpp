#include "DashSegmenterVideoSource.hh"
#include "SinkManager.hh"
#include <iostream>


DashSegmenterVideoSource* DashSegmenterVideoSource::createNew(UsageEnvironment& env, FramedSource* source, uint32_t frameRate, uint32_t segmentTime)
{
	return new DashSegmenterVideoSource(env, source, frameRate);
}

DashSegmenterVideoSource::DashSegmenterVideoSource(UsageEnvironment& env, FramedSource* source, uint32_t frameRate, uint32_t segmentTime): FramedSource(env), fInputSource(source)
{
	utils::debugMsg("Create");
	uint8_t i2error;
	i2error= context_initializer(&avContext, VIDEO_TYPE);
	if (i2error == I2ERROR_MEDIA_TYPE) {
		utils::errorMsg ("Media type incorrect");
	}
	set_segment_duration(segmentTime, &avContext);
	initFile = false;
	firstSample = true;
	currentTime.tv_sec = 0;
	currentTime.tv_usec = 0;
	initialTime.tv_sec = 0;
	initialTime.tv_usec = 0;
	previousTime.tv_sec = 0;
	previousTime.tv_sec = 0;
	semgentTime.tv_sec = 0;
	semgentTime.tv_usec = 0;
	spsSize = 0;
	ppsSize = 0;
	durationSampleFloat = 0.00;
	decodeTimeFloat = 0.00;
	totalSegmentDuration = 0;
	nal_data = new unsigned char[MAX_DAT];
	utils::debugMsg("end Create");
}

DashSegmenterVideoSource::~DashSegmenterVideoSource() {
	doStopGettingFrames();
	delete avContext;
}

void DashSegmenterVideoSource::doGetNextFrame(){
	utils::debugMsg("doGetNextFrame");
	printf ("fMaxSize %d\n", fMaxSize);
	fInputSource->getNextFrame(nal_data, fMaxSize, afterGettingFrame, this, FramedSource::handleClosure, this);
	utils::debugMsg("end doGetNextFrame");
}

void DashSegmenterVideoSource::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
	utils::debugMsg("afterGettingFrame");
	DashSegmenterVideoSource* source = (DashSegmenterVideoSource*)clientData;
	source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
	utils::debugMsg("end afterGettingFrame");
}

void DashSegmenterVideoSource::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
	unsigned char* buff = nal_data;
	uint32_t videoSample = frameSize;
	utils::debugMsg("afterGettingFrame1");
	if ((currentTime.tv_sec != presentationTime.tv_sec) || (currentTime.tv_usec != presentationTime.tv_usec)) {
		utils::debugMsg("New frame!");
		previousTime = currentTime;
		currentTime = presentationTime;
	}

	if (initFile == false) {
		if (frameSize >= 4 && nal_data[0] == 0 && nal_data[1] == 0 && ((nal_data[2] == 0 && nal_data[3] == 1) || nal_data[2] == 1)) {
			utils::debugMsg("MPEG 'start code' seen in the input");
			buff += 3;
			videoSample -= 3;
			if (nal_data[2] == 0 && nal_data[3] == 1) {
				buff++;
				videoSample--;
			}
		}
		if (spsSize == 0) {
			initialTime = presentationTime;
			previousTime = presentationTime;
			semgentTime = presentationTime;
			printf ("spsSize %u frameSize %u numTruncatedBytes %u\n", fFrameSize, frameSize, numTruncatedBytes);
			spsSize = frameSize;
			sps = new unsigned char[spsSize];
			printf("%u %u %u %u %u\n", nal_data[0], nal_data[1], nal_data[2], nal_data[3], nal_data[4]);
			memcpy(sps, nal_data+4, spsSize);
			printf("esto,\n");
			//segmentDuration = durationInMicroseconds;
			//return;
		}
		else if (ppsSize == 0) {
			ppsSize = fFrameSize;
			pps = new unsigned char[ppsSize];
			memcpy(pps, nal_data, ppsSize);
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
			firstSample = true;
			previousTime = fPresentationTime;
			memcpy(fTo, destinationData, initVideo);
			fFrameSize = initVideo;
			fNumTruncatedBytes = 0;
			fPresentationTime = semgentTime;
			fDurationInMicroseconds = totalSegmentDuration;
			totalSegmentDuration = 0;
			afterGetting(this);
		}
	}
	else {
		uint8_t isIntra = 0;
		uint32_t decodeT = decodeTime(previousTime, initialTime);
		uint32_t segmentD = segmentDuration(currentTime, previousTime);
		printf("Se agrega segmento con decodeTime: %u y segmentTime: %u\n", decodeT, segmentD);
		unsigned char* destinationData = new unsigned char [MAX_DAT];
		unsigned char* buff = nal_data;
		if (firstSample) {
			semgentTime = previousTime;
			firstSample = false;
		}
		if (((buff[0] & INTRA_MASK) == IDR_NAL) || ((buff[0] & INTRA_MASK) == SEI_NAL)) {
			isIntra = 1;
			utils::debugMsg ("Es intra!");
		}
		videoSample = add_sample(nal_data, fFrameSize, segmentD, decodeT, VIDEO_TYPE, destinationData, isIntra, &avContext);
		if (videoSample <= I2ERROR_MAX) {
			delete destinationData;
		}
		else {
			memcpy(fTo, destinationData, videoSample);
			firstSample = true;
			fFrameSize = videoSample;
			fNumTruncatedBytes = 0;
			fPresentationTime = semgentTime;
			fDurationInMicroseconds = totalSegmentDuration;
			totalSegmentDuration = 0;
			afterGetting(this);
		}
	}

	utils::debugMsg ("next frame!");
	doGetNextFrame();
	return;
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
	totalSegmentDuration += durationTime;
	return durationTime;
}

