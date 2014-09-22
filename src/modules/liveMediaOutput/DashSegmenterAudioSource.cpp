#include "DashSegmenterAudioSource.hh"
#include <iostream>


DashSegmenterAudioSource* DashSegmenterAudioSource::createNew(UsageEnvironment& env, FramedSource* source, bool reInit, uint32_t segmentTime, uint32_t sampleRate)
{
	return new DashSegmenterAudioSource(env, source, reInit, segmentTime, sampleRate);
}

DashSegmenterAudioSource::DashSegmenterAudioSource(UsageEnvironment& env, FramedSource* source, bool reInit, uint32_t segmentTime, uint32_t sampleRate): FramedSource(env), fInputSource(source), fSampleRate(sampleRate)
{
	uint8_t i2error;
	printf ("AUDIO TYPE %u\n", sampleRate);
	i2error= context_initializer(&avContext, AUDIO_TYPE);
	if (i2error == I2ERROR_MEDIA_TYPE) {
		printf ("Media type incorrect\n");
	}
	set_segment_duration(2, &avContext);
	set_sample_rate(sampleRate, &avContext);
	this->initFile = false;
	this->reInitFile = reInit;
	this->previousTime.tv_sec = 0;
	this->previousTime.tv_usec = 0;
	this->sampleDurationFloat = 0.00;
	this->decodeTimeFloat = 0.00;
	this->decodeTime = 0;
	this->totalSegmentDuration = 0;
	this->aacData = new unsigned char[MAX_AAC_SAMPLE];
	this->sampleData = new unsigned char[MAX_AAC_SAMPLE];
	this->offset = AAC_LENGTH_SIZE;
}

DashSegmenterAudioSource::~DashSegmenterAudioSource() {
	doStopGettingFrames();
	delete avContext;
}

void DashSegmenterAudioSource::doGetNextFrame(){
	fInputSource->getNextFrame(aacData + offset, fMaxSize, afterGettingFrame, this, FramedSource::handleClosure, this);
}

void DashSegmenterAudioSource::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
	DashSegmenterAudioSource* source = (DashSegmenterAudioSource*)clientData;
	source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void DashSegmenterAudioSource::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
	long int sampleDuration = (1024*1000) / fSampleRate;
	uint32_t audioSample;
	uint32_t aacSizeHtoN = htonl(frameSize);
	memcpy(aacData, &aacSizeHtoN, AAC_LENGTH_SIZE);
	//offset+=NAL_LENGTH_SIZE;

	sampleDurationFloat +=  (float) (1024*1000) / (float) fSampleRate;
	sampleDurationFloat -= sampleDuration;
	if (sampleDurationFloat >= 1) {
		sampleDurationFloat--;
		sampleDuration++;
	}
	
	audioSample = add_sample(aacData, (frameSize + offset), sampleDuration, decodeTime, AUDIO_TYPE, fTo, 1, &avContext);
	previousTime = presentationTime;
	decodeTime += sampleDuration;
	if (audioSample <= I2ERROR_MAX) {
		totalSegmentDuration += sampleDuration;
	}
	else {
		sampleDurationFloat = 0.00;
		decodeTimeFloat = 0.00;
		fFrameSize = audioSample;
		fNumTruncatedBytes = 0;
		fPresentationTime = segmentTime;
		fDurationInMicroseconds = totalSegmentDuration;
		totalSegmentDuration = sampleDuration;
		segmentTime = presentationTime;
		afterGetting(this);
		return;
	}
	printf("new data %u %u %lu %lu\n", durationInMicroseconds, frameSize, presentationTime.tv_sec, presentationTime.tv_usec);
	if ((previousTime.tv_sec == 0) && (previousTime.tv_usec == 0)) {
		previousTime = presentationTime;
	}
	init = false;


	if (!initFile) {
		unsigned char metadata[4];
		uint32_t metadataSize = 4, initAudio = 0;    	
		metadata[0] = 0xEB;
    	metadata[1] = 0x8A;
    	metadata[2] = 0x08;
    	metadata[3] = 0x00;
    	printf("init\n");
		initAudio = init_audio_handler(metadata, metadataSize, fTo, &avContext);
		initFile = true;
		fFrameSize = initAudio;
		fNumTruncatedBytes = 0;
		fPresentationTime = presentationTime;
		fDurationInMicroseconds = 0;
		segmentTime = presentationTime;
		init = true;
		printf("init!!!! %u\n", initAudio);
		afterGetting(this);
		printf("hola!\n");
		return;
	}
	doGetNextFrame();
	return;	
/*
	unsigned char nalType = nalData[0] & NAL_TYPE_MASK;
	uint32_t videoSample = frameSize;
	init = false;
	switch (nalType) {
	case SEI_NAL: //SEI
		if (seiSize)
			delete sei;
		seiSize = frameSize;
		sei = new unsigned char[seiSize];
		memcpy(sei, nalData, seiSize);
		isIntra = 1;
		break;
	case SPS_NAL: //SPS
		segmentTime = presentationTime;
		previousTime = presentationTime;
		if (spsSize)
			delete sps;
		spsSize = frameSize;
		sps = new unsigned char[spsSize];
		memcpy(sps, nalData, spsSize);
		break;
	case PPS_NAL: //PPS
		if (ppsSize)
			delete pps;
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
		
			initVideo = init_video_handler(metadata, metadataSize, metadata2, metadata2Size, sps, &spsSize, metadata3, metadata3Size, pps, ppsSize, fTo, &avContext);
			initFile = true;
			fFrameSize = initVideo;
			fNumTruncatedBytes = 0;
			fPresentationTime = segmentTime;
			fDurationInMicroseconds = 0;
			segmentTime = presentationTime;
			init = true;
			afterGetting(this);
			return;
		}
		break;
	case IDR_NAL: //IDR
		isIntra = 1;
	default: //OTHER TYPE OF NALS
		long int durationSam = (((presentationTime.tv_sec*1000000) + presentationTime.tv_usec)-((previousTime.tv_sec*1000000) + previousTime.tv_usec));
		if (durationSam == 0) {//buffering*/
			/*
			+-----------+---------------+-----------+---------------+-----------+---------------+-----------+---------------+
			|Nal 1 Size |     Nal 1     |Nal 2 Size |     Nal 2     |    ....   |     .....     |Nal N Size |     Nal N     |
			+-----------+---------------+-----------+---------------+-----------+---------------+-----------+---------------+
			*/
			/*uint32_t nalSizeHtoN = htonl(frameSize);
			memcpy(sampleData + offset, &nalSizeHtoN, NAL_LENGTH_SIZE);
			offset+=NAL_LENGTH_SIZE;
			memcpy(sampleData + offset, nalData, frameSize);
			offset+= frameSize;			
		}
		else {
			uint32_t nalSizeHtoN = htonl(frameSize);
			uint32_t sampleDuration = durationSam / 1000;
			sampleDurationFloat +=  (float) durationSam / 1000.00;
			sampleDurationFloat -= sampleDuration;
			if (sampleDurationFloat >= 1) {
				sampleDurationFloat--;
				sampleDuration++;
			}
			videoSample = add_sample(sampleData, offset, sampleDuration, decodeTime, VIDEO_TYPE, fTo, isIntra, &avContext);
			previousTime = presentationTime;
			isIntra = 1;
			offset = 0;
			memcpy(sampleData + offset, &nalSizeHtoN, NAL_LENGTH_SIZE);
			offset+=NAL_LENGTH_SIZE;
			memcpy(sampleData + offset, nalData, frameSize);
			offset+= frameSize;			
			decodeTime += sampleDuration;
			if (videoSample <= I2ERROR_MAX) {
				totalSegmentDuration += sampleDuration;
			}
			else {
				sampleDurationFloat = 0.00;
				decodeTimeFloat = 0.00;
				fFrameSize = videoSample;
				fNumTruncatedBytes = 0;
				fPresentationTime = segmentTime;
				fDurationInMicroseconds = totalSegmentDuration;
				totalSegmentDuration = sampleDuration;
				segmentTime = presentationTime;
				afterGetting(this);
				return;
			}
		}
		break;
	}*/

}

void DashSegmenterAudioSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}

