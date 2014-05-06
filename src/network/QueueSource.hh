
#ifndef _QUEUE_SOURCE_HH
#define _QUEUE_SOURCE_HH

#ifndef _LIVEMEDIA_HH
#include <liveMedia.hh>
#endif

#ifndef _FRAME_QUEUE_HH
#include "../FrameQueue.hh"
#endif

#ifndef _FRAME_HH
#include "../Frame.hh"
#endif

class QueueSource: public FramedSource {

public:
    static QueueSource* createNew(UsageEnvironment& env, FrameQueue *q);
    virtual void doGetNextFrame();
    virtual void doStopGettingFrames();


protected:
    QueueSource(UsageEnvironment& env, FrameQueue *q);
        // called only by createNew()

private: // redefined virtual functions:
    static void staticDoGetNextFrame(FramedSource* source);

private:
    FrameQueue* queue;
    Frame* frame;
    bool has_frame;
};

#endif