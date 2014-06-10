#include "QueueSource.hh"
#include <iostream>

QueueSource* QueueSource::createNew(UsageEnvironment& env, FrameQueue *q) {
  return new QueueSource(env, q);
}


QueueSource::QueueSource(UsageEnvironment& env, FrameQueue *q)
  : FramedSource(env) {
    setQueue(q);
}

void QueueSource::doGetNextFrame() {
    if ((frame = getFrame()) == NULL) {
        nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
            (TaskFunc*)QueueSource::staticDoGetNextFrame, this);
        return;
    }
    
    fPresentationTime = frame->getPresentationTime();
    if (fMaxSize < frame->getLength()){
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = frame->getLength() - fMaxSize;
    } else {
        fNumTruncatedBytes = 0; 
        fFrameSize = frame->getLength();
    }
    
    memcpy(fTo, frame->getDataBuf(), fFrameSize);
    removeFrame();
    
    afterGetting(this);
}

void QueueSource::doStopGettingFrames() {
    return;
}

void QueueSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}

