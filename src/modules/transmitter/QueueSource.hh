#ifndef _QUEUE_SOURCE_HH
#define _QUEUE_SOURCE_HH

#include <liveMedia.hh>

#include "../../FrameQueue.hh"
#include "../../Frame.hh"

#define POLL_TIME 1000

class QueueSource: public FramedSource {

public:
    static QueueSource* createNew(UsageEnvironment& env, const StreamInfo* streamInfo);
    bool setFrame(Frame *f);
    bool gotFrame();
    EventTriggerId getTriggerId() const {return eventTriggerId;};
    static bool signalNewFrameData(TaskScheduler* ourScheduler, QueueSource* ourSource);

protected:
    void doGetNextFrame();
    QueueSource(UsageEnvironment& env, const StreamInfo* streamInfo);
    static void deliverFrame0(void* clientData);
    virtual void deliverFrame();
    void doStopGettingFrames();

protected:
    EventTriggerId eventTriggerId;
    Frame* frame;
    const StreamInfo* si;
    bool processedFrame;
    bool stopFrames;
};

#endif
