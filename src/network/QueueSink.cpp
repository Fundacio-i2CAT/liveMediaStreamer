#include "QueueSink.hh"
#include <iostream>
#include <sys/time.h>

#define CODED_VIDEO_FRAMES 512 
#define CODED_AUDIO_FRAMES 1024
#define MAX_AUDIO_FRAME_SIZE 2048
#define MAX_VIDEO_FRAME_SIZE 100000

QueueSink::QueueSink(UsageEnvironment& env, char const* mediumName)
  : MediaSink(env) {
    if (strcmp(mediumName, "audio") == 0) {
        queue = FrameQueue::createNew(CODED_AUDIO_FRAMES, MAX_AUDIO_FRAME_SIZE, 0);
    } else if (strcmp(mediumName, "video") == 0) {
        queue = FrameQueue::createNew(CODED_VIDEO_FRAMES, MAX_VIDEO_FRAME_SIZE, 0);
    }   
}

QueueSink* QueueSink::createNew(UsageEnvironment& env, char const* mediumName) 
{
    return new QueueSink(env, mediumName);
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
