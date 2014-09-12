#ifndef _DASH_SEGMENTER_VIDEO_SOURCE_HH
#define _DASH_SEGMENTER_VIDEO_SOURCE_HH

#include <liveMedia/FramedSource.hh>
#include <i2libdash.h>
#include <i2context.h>

#include "../../FrameQueue.hh"
#include "../../IOInterface.hh"
#include "../../Frame.hh"
#include "../../Utils.hh"

#define	IDR_NAL 5
#define SEI_NAL 6
#define SPS_NAL 7
#define PPS_NAL 8
#define NAL_TYPE_MASK 0x1F
#define NAL_LENGTH_SIZE 4

class DashSegmenterVideoSource: public FramedSource {

public:
    	static DashSegmenterVideoSource* createNew(UsageEnvironment& env, FramedSource* source, uint32_t frameRate = 24, uint32_t segmentTime = 5);
	virtual void doGetNextFrame();

protected:
    	DashSegmenterVideoSource(UsageEnvironment& env, FramedSource* source, uint32_t frameRate = 24, uint32_t segmentTime = 5);
	void checkStatus();
	static void staticDoGetNextFrame(FramedSource* source);
	virtual ~DashSegmenterVideoSource();
private:
	static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
	void afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes, struct timeval presentationTime, unsigned durationInMicroseconds);
	i2ctx* avContext;
	bool initFile;
	FramedSource* fInputSource;
	unsigned char* pps;
	unsigned char* sps;
	unsigned char* nal_data;
	unsigned char* intraData;
	uint32_t intraSize;
	uint32_t ppsSize;
	uint32_t spsSize;
	struct timeval segmentTime;
	struct timeval previousTime;
	float sampleDurationFloat;
	float decodeTimeFloat;
	uint32_t decodeTime;
	uint32_t totalSegmentDuration;
};

#endif
