#ifndef _QUEUE_SOURCE_HH
#define _QUEUE_SOURCE_HH

#include <liveMedia.hh>

#include "../../FrameQueue.hh"
#include "../../IOInterface.hh"
#include "../../Frame.hh"

#define POLL_TIME 1000

class QueueSource: public FramedSource {

public:
    static QueueSource* createNew(UsageEnvironment& env, Reader *reader, int readerId);
    virtual void doGetNextFrame();
    Reader* getReader() {return fReader;};

protected:
    QueueSource(UsageEnvironment& env, Reader *reader, int readerId);
        // called only by createNew()
    static void staticDoGetNextFrame(FramedSource* source);
    void checkStatus();

protected:
    Frame* frame;
    Reader *fReader;
    int fReaderId;
};

#endif
