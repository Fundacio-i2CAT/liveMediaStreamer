#include "DashSegmenterVideoSource.hh"
#include "SinkManager.hh"
#include <iostream>


DashSegmenterVideoSource* DashSegmenterVideoSource::createNew(UsageEnvironment& env, FramedSource* source, bool reInit, uint32_t frameRate, uint32_t segmentTime)
{
	return new DashSegmenterVideoSource(env, source, reInit, frameRate, segmentTime);
}

DashSegmenterVideoSource::DashSegmenterVideoSource(UsageEnvironment& env, FramedSource* source, bool reInit, uint32_t frameRate, uint32_t segmentTime): FramedSource(env), fInputSource(source)
{
	uint8_t i2error;
	i2error= context_initializer(&avContext, VIDEO_TYPE);
	if (i2error == I2ERROR_MEDIA_TYPE) {
		printf ("Media type incorrect\n");
	}
	set_segment_duration(segmentTime, &avContext);
	this->initFile = false;
	this->reInitFile = reInit;
	this->spsSize = 0;
	this->ppsSize = 0;
	this->seiSize = 0;
	this->sampleDurationFloat = 0.00;
	this->decodeTimeFloat = 0.00;
	this->decodeTime = 0;
	this->totalSegmentDuration = 0;
	this->intraSize = 0;
	this->nalData = new unsigned char[MAX_DAT];
}

DashSegmenterVideoSource::~DashSegmenterVideoSource() {
	doStopGettingFrames();
	delete avContext;
}

void DashSegmenterVideoSource::doGetNextFrame(){
	fInputSource->getNextFrame(nalData, fMaxSize, afterGettingFrame, this, FramedSource::handleClosure, this);
}

void DashSegmenterVideoSource::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
	DashSegmenterVideoSource* source = (DashSegmenterVideoSource*)clientData;
	source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void DashSegmenterVideoSource::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
	unsigned char nalType = nalData[0] & NAL_TYPE_MASK;
	uint8_t isIntra = 0;
	unsigned char *destinationData;
	uint32_t videoSample = frameSize;
	
	switch (nalType) {
	case SEI_NAL: //SEI
		seiSize = frameSize;
		sei = new unsigned char[seiSize];
		memcpy(sei, nalData, seiSize);
		break;
	case SPS_NAL: //SPS
		segmentTime = presentationTime;
		spsSize = frameSize;
		sps = new unsigned char[spsSize];
		memcpy(sps, nalData, spsSize);
		break;
	case PPS_NAL: //PPS
		ppsSize = frameSize;
		pps = new unsigned char[ppsSize];
		memcpy(pps, nalData, ppsSize);
		
		if (((initFile == false) || (reInitFile == true)) && ((spsSize > 0) && (ppsSize > 0))) {
			unsigned char *metadata, *metadata2, *metadata3;
			uint32_t metadataSize = 0, metadata2Size = 0, metadata3Size = 0, initVideo = 0;
			// METADATA
			metadata = new unsigned char[4];
			metadata[0] = 0x01; metadata[1] = 0x42; metadata[2] = 0xC0; metadata[3] = 0x1E;
			metadataSize = 4;
			// METADATA2
			metadata2 = new unsigned char[2];
			metadata2[0] = 0xFF; metadata2[1] = 0xE1;
			metadata2Size = 2;
			// METADATA3
			metadata3 = new unsigned char[1];
			metadata3[0] = 0x01;
			metadata3Size = 1;
			// DESTINATION DATA
			destinationData = new unsigned char [MAX_INIT_FILE_SIZE];
		
			initVideo = init_video_handler(metadata, metadataSize, metadata2, metadata2Size, sps, &spsSize, metadata3, metadata3Size, pps, ppsSize, destinationData, &avContext);
			initFile = true;
			memcpy(fTo, destinationData, initVideo);
			fFrameSize = initVideo;
			fNumTruncatedBytes = 0;
			fPresentationTime = segmentTime;
			fDurationInMicroseconds = 0;
			segmentTime = presentationTime;
			afterGetting(this);
			return;
		}
		break;
	case IDR_NAL: //IDR
		isIntra = 1;
	default:
		unsigned char* nalDataWithSize = new unsigned char [frameSize + NAL_LENGTH_SIZE];
		uint32_t nalSizeHtoN = htonl(frameSize);
		uint32_t sampleDuration = durationInMicroseconds / 1000;
		sampleDurationFloat +=  (float) durationInMicroseconds / 1000.00;
		sampleDurationFloat -= sampleDuration;
		if (sampleDurationFloat >= 1) {
			sampleDurationFloat--;
			sampleDuration++;
		}
		destinationData = new unsigned char [MAX_DAT];
		memcpy(nalDataWithSize, &nalSizeHtoN, NAL_LENGTH_SIZE);
		memcpy(nalDataWithSize+NAL_LENGTH_SIZE, nalData, frameSize);
		videoSample = add_sample(nalDataWithSize, frameSize + NAL_LENGTH_SIZE, sampleDuration, decodeTime, VIDEO_TYPE, destinationData, isIntra, &avContext);
		decodeTime += sampleDuration;
		if (videoSample <= I2ERROR_MAX) {
			totalSegmentDuration += durationInMicroseconds;
			delete destinationData;
		}
		else {
			memcpy(fTo, destinationData, videoSample);
			delete destinationData;
			sampleDurationFloat = 0.00;
			decodeTimeFloat = 0.00;
			fFrameSize = videoSample;
			fNumTruncatedBytes = 0;
			fPresentationTime = segmentTime;
			fDurationInMicroseconds = totalSegmentDuration;
			totalSegmentDuration = durationInMicroseconds;
			segmentTime = presentationTime;
			afterGetting(this);
			return;
		}
		break;
	}
	doGetNextFrame();
	return;
}

void DashSegmenterVideoSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}

