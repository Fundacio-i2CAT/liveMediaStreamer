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
}

DashSegmenterAudioSource::~DashSegmenterAudioSource() {
	doStopGettingFrames();
	delete avContext;
}

void DashSegmenterAudioSource::doGetNextFrame(){
	fInputSource->getNextFrame(aacData, fMaxSize, afterGettingFrame, this, FramedSource::handleClosure, this);
}

void DashSegmenterAudioSource::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
	DashSegmenterAudioSource* source = (DashSegmenterAudioSource*)clientData;
	source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void DashSegmenterAudioSource::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds) {
	long int sampleDuration = (1024*1000*2) / fSampleRate;
	uint32_t audioSample;

	sampleDurationFloat +=  (float) (1024*1000*2) / (float) fSampleRate;
	sampleDurationFloat -= sampleDuration;
	if (sampleDurationFloat >= 1) {
		sampleDurationFloat--;
		sampleDuration++;
	}
	
	audioSample = add_sample(aacData, frameSize, sampleDuration, decodeTime, AUDIO_TYPE, fTo, 1, &avContext);
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
		initAudio = init_audio_handler(metadata, metadataSize, fTo, &avContext);
		initFile = true;
		fFrameSize = initAudio;
		fNumTruncatedBytes = 0;
		fPresentationTime = presentationTime;
		fDurationInMicroseconds = 0;
		segmentTime = presentationTime;
		init = true;
		afterGetting(this);
		return;
	}

	doGetNextFrame();
	return;
}

void DashSegmenterAudioSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}

