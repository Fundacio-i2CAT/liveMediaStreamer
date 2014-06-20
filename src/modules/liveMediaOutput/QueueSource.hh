#ifndef _QUEUE_SOURCE_HH
#define _QUEUE_SOURCE_HH

#include <liveMedia/liveMedia.hh>

#include "../../FrameQueue.hh"
#include "../../IOInterface.hh"
#include "../../Frame.hh"

#define POLL_TIME 1000

class QueueSource: public FramedSource {

public:
    static QueueSource* createNew(UsageEnvironment& env, Reader *reader);
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();
    Reader* getReader() {return fReader;};

protected:
    QueueSource(UsageEnvironment& env, Reader *reader);
        // called only by createNew()
    static void staticDoGetNextFrame(FramedSource* source);

protected:
    Frame* frame;
    Reader *fReader;
};

#endif
