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
	spsSize = 0;
	ppsSize = 0;
	sampleDurationFloat = 0.00;
	decodeTimeFloat = 0.00;
	decodeTime = 0;
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
	printf ("durationInMicroseconds %u, sec %lu, usec %lu \n", durationInMicroseconds, presentationTime.tv_sec, presentationTime.tv_usec);
	if (initFile == false) {
		if ((buff[0] & NAL_TYPE_MASK) == SPS_NAL) {
			segmentTime = presentationTime;
			printf("SPS\n");
			printf ("spsSize %u frameSize %u numTruncatedBytes %u\n", fFrameSize, frameSize, numTruncatedBytes);
			spsSize = frameSize;
			sps = new unsigned char[spsSize];
			printf("%u %u %u %u %u\n", buff[0], buff[1], buff[2], buff[3], buff[4]);
			memcpy(sps, buff, spsSize);
			//segmentDuration = durationInMicroseconds;
			//return;
		}
		else if ((buff[0] & NAL_TYPE_MASK) == PPS_NAL) {
			ppsSize = frameSize;
			pps = new unsigned char[ppsSize];
			memcpy(pps, buff, ppsSize);
			printf("PPS\n");
			printf ("ppsSize %u frameSize %u numTruncatedBytes %u\n", fFrameSize, frameSize, numTruncatedBytes);
			printf("%u %u %u %u %u\n", buff[0], buff[1], buff[2], buff[3], buff[4]);
		} else if ((buff[0] & NAL_TYPE_MASK) == SEI_NAL){
			uint8_t isIntra = 1;
			unsigned char* destinationData = new unsigned char [MAX_DAT];
			uint32_t sampleDuration = durationInMicroseconds / 1000;
			sampleDurationFloat +=  (float) durationInMicroseconds / 1000.00;
			sampleDurationFloat -= sampleDuration;
			if (sampleDurationFloat >= 1) {
				sampleDurationFloat--;
				sampleDuration++;
			}
			decodeTime = 0;
			totalSegmentDuration = 0;
			printf ("sampleDuration %u, decodeTime %u, totalSegmentDuration %u\n", sampleDuration, decodeTime, totalSegmentDuration);
			videoSample = add_sample(buff, fFrameSize, sampleDuration, decodeTime, VIDEO_TYPE, destinationData, isIntra, &avContext);
			delete destinationData;

		}
		if ((spsSize > 0) && (ppsSize > 0)) {
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
			memcpy(fTo, destinationData, initVideo);
			fFrameSize = initVideo;
			fNumTruncatedBytes = 0;
			fPresentationTime = segmentTime;
			fDurationInMicroseconds = totalSegmentDuration;
			totalSegmentDuration = 0;
			segmentTime = presentationTime;
			afterGetting(this);
			return;
		}
	}
	else {
		uint8_t isIntra = 0;
		unsigned char* destinationData = new unsigned char [MAX_DAT];
		uint32_t sampleDuration = durationInMicroseconds / 1000;
		sampleDurationFloat +=  (float) durationInMicroseconds / 1000.00;
		sampleDurationFloat -= sampleDuration;
		if (sampleDurationFloat >= 1) {
			sampleDurationFloat--;
			sampleDuration++;
		}
		decodeTime += durationInMicroseconds / 1000;
		decodeTimeFloat +=  (float) durationInMicroseconds / 1000.00;
		decodeTimeFloat -= decodeTime;
		if (decodeTimeFloat >= 0.5) {
			decodeTimeFloat--;
			decodeTime++;
		}
		if (((buff[0] & NAL_TYPE_MASK) == IDR_NAL) || ((buff[0] & NAL_TYPE_MASK) == SEI_NAL)) {
			isIntra = 1;
			utils::debugMsg ("Es intra!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		}
		printf ("sampleDuration %u, decodeTime %u, totalSegmentDuration %u\n", sampleDuration, decodeTime, totalSegmentDuration);
		videoSample = add_sample(buff, frameSize, sampleDuration, decodeTime, VIDEO_TYPE, destinationData, isIntra, &avContext);
		if (videoSample <= I2ERROR_MAX) {
			utils::debugMsg ("no segment");
			totalSegmentDuration += durationInMicroseconds;
			delete destinationData;
		}
		else {
			utils::debugMsg ("NEW SEGMENT!\n");
			memcpy(fTo, destinationData, videoSample);
			delete destinationData;
			fFrameSize = videoSample;
			fNumTruncatedBytes = 0;
			fPresentationTime = segmentTime;
			fDurationInMicroseconds = totalSegmentDuration;
			totalSegmentDuration = durationInMicroseconds;
			segmentTime = presentationTime;
			afterGetting(this);
			return;
		}
	}
	utils::debugMsg ("END frame!\n");
	doGetNextFrame();
	return;
}

void DashSegmenterVideoSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}

