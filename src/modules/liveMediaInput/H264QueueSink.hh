#ifndef _H264_QUEUE_SINK_HH
#define _H264_QUEUE_SINK_HH

#include <liveMedia.hh>

#include "QueueSink.hh"


class H264QueueSink: public QueueSink {

public:
    static H264QueueSink* createNew(UsageEnvironment& env,
                                    unsigned port, char const* sPropParameterSetsStr);

protected:
    H264QueueSink(UsageEnvironment& env,
                  unsigned port, char const* sPropParameterSetsStr);

    void afterGettingFrame(unsigned frameSize, struct timeval presentationTime);
    Boolean continuePlaying();

private:

    char const* fSPropParameterSetsStr;
    Boolean fHaveWrittenFirstFrame;
    unsigned offset;
};

#endif
