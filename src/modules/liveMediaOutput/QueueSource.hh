#ifndef _QUEUE_SOURCE_HH
#define _QUEUE_SOURCE_HH

#include <liveMedia.hh>

#include "../../FrameQueue.hh"
#include "../../Frame.hh"

#define POLL_TIME 1000

class QueueSource: public FramedSource {

public:
    static QueueSource* createNew(UsageEnvironment& env, int readerId);
    bool setFrame(Frame *f);
    Frame* getFrame();
    EventTriggerId getTriggerId() const {return eventTriggerId;};
    static bool signalNewFrameData(TaskScheduler* ourScheduler, QueueSource* ourSource);

protected:
    void doGetNextFrame();
    QueueSource(UsageEnvironment& env, int readerId);
    static void deliverFrame0(void* clientData);
    virtual void deliverFrame();

protected:
    EventTriggerId eventTriggerId;
    Frame* frame;
    int fReaderId;
    bool processedFrame;
};

#endif
