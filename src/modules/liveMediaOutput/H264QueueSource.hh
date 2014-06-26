
#ifndef _H264_QUEUE_SOURCE_HH
#define _H264_QUEUE_SOURCE_HH

#include <liveMedia/liveMedia.hh>
#include "QueueSource.hh"


class H264QueueSource: public QueueSource {

public:
    static H264QueueSource* createNew(UsageEnvironment& env, Reader *reader, int readerId);
    virtual void doGetNextFrame();


protected:
    H264QueueSource(UsageEnvironment& env, Reader *reader, int readerId);
};

#endif
