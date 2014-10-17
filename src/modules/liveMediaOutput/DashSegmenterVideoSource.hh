#ifndef _DASH_SEGMENTER_VIDEO_SOURCE_HH
#define _DASH_SEGMENTER_VIDEO_SOURCE_HH

#include <liveMedia/FramedSource.hh>
#include <i2libdash.h>
#include <i2context.h>

#define	IDR_NAL 5
#define SEI_NAL 6
#define SPS_NAL 7
#define PPS_NAL 8
#define NAL_TYPE_MASK 0x1F
#define NAL_LENGTH_SIZE 4
#define FRAME_RATE 25
#define SEGMENT_TIME 2 //seconds
#define MAX_H264_SAMPLE 1024*1024 //1MB

class DashSegmenterVideoSource: public FramedSource {

public:
   	static DashSegmenterVideoSource* createNew(UsageEnvironment& env, FramedSource* source, bool reInit = false, uint32_t frameRate = FRAME_RATE, uint32_t segmentTime = SEGMENT_TIME);
	virtual void doGetNextFrame();
	bool isInit(){return init;};

protected:
    DashSegmenterVideoSource(UsageEnvironment& env, FramedSource* source, bool reInit = false, uint32_t frameRate = FRAME_RATE, uint32_t segmentTime = SEGMENT_TIME);
	void checkStatus();
	static void staticDoGetNextFrame(FramedSource* source);
	virtual ~DashSegmenterVideoSource();
private:
	static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
	void afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
	i2ctx* avContext;
	bool initFile;
	bool reInitFile;
	bool init;
	FramedSource* fInputSource;
	unsigned char* pps;
	unsigned char* sps;
	unsigned char* sei;
	unsigned char* nalData;
	unsigned char* sampleData;
	uint32_t offset;
	uint32_t intraSize;
	uint32_t ppsSize;
	uint32_t spsSize;
	uint32_t seiSize;
	struct timeval segmentTime;
	float sampleDurationFloat;
	float decodeTimeFloat;
	uint32_t decodeTime;
	uint32_t totalSegmentDuration;
	struct timeval previousTime;
	uint8_t isIntra;
};

#endif
