#include "QueueSink.hh"
#include <iostream>
#include <sys/time.h>

QueueSink::QueueSink(UsageEnvironment& env, FrameQueue *q)
  : MediaSink(env), queue(q) {}

QueueSink* QueueSink::createNew(UsageEnvironment& env, FrameQueue *q) 
{
    return new QueueSink(env, q);
}

Boolean QueueSink::continuePlaying() 
{
    if (fSource == NULL) return False;

    //Check if there is any free slot in the queue
    while ((frame = queue->getRear()) == NULL) {
        std::cerr << "Queue overflow, flushing frames! Increase queue max position in order to fix this!" << std::endl;
        queue->flush();
    }

    fSource->getNextFrame(frame->getDataBuf(), frame->getMaxLength(),
              afterGettingFrame, this,
              onSourceClosure, this);

    return True;
}

void QueueSink::afterGettingFrame(void* clientData, unsigned frameSize,
                 unsigned numTruncatedBytes,
                 struct timeval presentationTime,
                 unsigned /*durationInMicroseconds*/) 
{
  QueueSink* sink = (QueueSink*)clientData;
  sink->afterGettingFrame(frameSize, presentationTime);
}

void QueueSink::afterGettingFrame(unsigned frameSize, struct timeval presentationTime) 
{
    frame->setLength(frameSize);
    frame->setUpdatedTime();
    frame->setPresentationTime(presentationTime);
    queue->addFrame();

    // Then try getting the next frame:
    continuePlaying();
}
