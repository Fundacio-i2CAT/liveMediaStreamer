#ifndef _DASH_SEGMENTER_AUDIO_SOURCE_HH
#define _DASH_SEGMENTER_AUDIO_SOURCE_HH

#include <liveMedia/FramedSource.hh>
#include <i2libdash.h>
#include <i2context.h>

#define	IDR_NAL 5
#define SEI_NAL 6
#define SPS_NAL 7
#define PPS_NAL 8
#define NAL_TYPE_MASK 0x1F
#define AAC_LENGTH_SIZE 4
#define SAMPLE_RATE 48000
#define SEGMENT_TIME 2 //seconds
#define MAX_AAC_SAMPLE 1024*1024 //1MB

class DashSegmenterAudioSource: public FramedSource {

public:
   	static DashSegmenterAudioSource* createNew(UsageEnvironment& env, FramedSource* source, bool reInit = false, uint32_t segmentTime = SEGMENT_TIME, uint32_t sampleRate = SAMPLE_RATE);
	virtual void doGetNextFrame();
	bool isInit(){return init;};

protected:
    DashSegmenterAudioSource(UsageEnvironment& env, FramedSource* source, bool reInit = false, uint32_t segmentTime = SEGMENT_TIME, uint32_t frameRate = SAMPLE_RATE);
	void checkStatus();
	static void staticDoGetNextFrame(FramedSource* source);
	virtual ~DashSegmenterAudioSource();
private:
	static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
	void afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
	i2ctx* avContext;
	bool initFile;
	bool reInitFile;
	bool init;
	FramedSource* fInputSource;
	unsigned char* aacData;
	unsigned char* sampleData;
	uint32_t fSampleRate;
	float sampleDurationFloat;
	uint32_t decodeTime;
	uint32_t totalSegmentDuration;
	struct timeval segmentTime;
	struct timeval previousTime;
	uint8_t isIntra;
};

#endif
