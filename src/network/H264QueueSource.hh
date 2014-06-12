
#ifndef _H264_QUEUE_SOURCE_HH
#define _H264_QUEUE_SOURCE_HH

#ifndef _LIVEMEDIA_HH
#include <liveMedia/liveMedia.hh>
#endif

#ifndef _QUEUE_SOURCE_HH
#include "QueueSource.hh"
#endif

class H264QueueSource: public QueueSource {

public:
    static H264QueueSource* createNew(UsageEnvironment& env, Reader *reader);
    virtual void doGetNextFrame();


protected:
    H264QueueSource(UsageEnvironment& env, Reader *reader);
};

#endif
