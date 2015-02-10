#ifndef _H264_QUEUE_SINK_HH
#define _H264_QUEUE_SINK_HH

#include <liveMedia.hh>

#include "QueueSink.hh"


class H264QueueSink: public QueueSink {

public:
    static H264QueueSink* createNew(UsageEnvironment& env, Writer *writer,
                                    unsigned port, char const* sPropParameterSetsStr);

protected:
    H264QueueSink(UsageEnvironment& env, Writer *writer,
                  unsigned port, char const* sPropParameterSetsStr);

    Boolean continuePlaying();
    void afterGettingFrame(unsigned frameSize, struct timeval presentationTime);

private:

    char const* fSPropParameterSetsStr;
    Boolean fHaveWrittenFirstFrame;
};

#endif
