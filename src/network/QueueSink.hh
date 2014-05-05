#ifndef _QUEUE_SINK_HH
#define _QUEUE_SINK_HH

#include <liveMedia.hh>
#ifndef _FRAME_QUEUE_HH
#include "../Frame.hh"
#include "../FrameQueue.hh"
#endif

class QueueSink: public MediaSink {

public:
    static QueueSink* createNew(UsageEnvironment& env, char const* mediumName);

protected:
    QueueSink(UsageEnvironment& env, char const* mediumName);


protected: 
    virtual Boolean continuePlaying();

protected:
    static void afterGettingFrame(void* clientData, unsigned frameSize,
                unsigned numTruncatedBytes,
                struct timeval presentationTime,
                unsigned durationInMicroseconds);
    virtual void afterGettingFrame(unsigned frameSize, struct timeval presentationTime);

    FrameQueue* queue;
    Frame *frame;
};

#endif