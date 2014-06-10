#ifndef _QUEUE_SOURCE_HH
#define _QUEUE_SOURCE_HH

#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#ifndef _FRAME_QUEUE_HH
#include "../FrameQueue.hh"
#endif

#ifndef _IO_INTERFACE_HH
#include "../IOInterface.hh"
#endif

#ifndef _FRAME_HH
#include "../Frame.hh"
#endif

class QueueSource: public FramedSource, public Reader {

public:
    static QueueSource* createNew(UsageEnvironment& env, FrameQueue *q);
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();

protected:
    QueueSource(UsageEnvironment& env, FrameQueue *q);
        // called only by createNew()
    static void staticDoGetNextFrame(FramedSource* source);

protected:
    Frame* frame;
};

#endif
