#ifndef _DASH_SEGMENTER_VIDEO_SOURCE_HH
#define _DASH_SEGMENTER_VIDEO_SOURCE_HH

#include <liveMedia/FramedSource.hh>
#include <i2libdash.h>
#include <i2context.h>

#include "../../FrameQueue.hh"
#include "../../IOInterface.hh"
#include "../../Frame.hh"
#include "../../Utils.hh"

class DashSegmenterVideoSource: public FramedSource {

public:
    	static DashSegmenterVideoSource* createNew(UsageEnvironment& env, Reader *reader, int readerId, uint32_t frameRate = 24);
	virtual void doGetNextFrame();
	Reader* getReader() {return fReader;};

protected:
    	DashSegmenterVideoSource(UsageEnvironment& env, Reader *reader, int readerId, uint32_t frameRate = 24);
	void checkStatus();
	static void staticDoGetNextFrame(FramedSource* source);
	virtual ~DashSegmenterVideoSource();
private:	
	i2ctx* avContext;
	bool initFile;
	Reader *fReader;
	int fReaderId;
	FramedSource* fInputSource;
	Frame* frame;
	unsigned char* pps;
	unsigned char* sps;
	uint32_t ppsSize;
	uint32_t spsSize;
	struct timeval presentationTime;
	struct timeval previousTime;
};

#endif
