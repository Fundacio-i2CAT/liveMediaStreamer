#include "QueueSource.hh"
#include <iostream>

QueueSource* QueueSource::createNew(UsageEnvironment& env, Reader *reader) {
  return new QueueSource(env, reader);
}


QueueSource::QueueSource(UsageEnvironment& env, Reader *reader)
  : FramedSource(env), fReader(reader) {
}

void QueueSource::doGetNextFrame() {
    if ((frame = fReader->getFrame()) == NULL) {
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
    fReader->removeFrame();
    
    afterGetting(this);
}

void QueueSource::doStopGettingFrames() {
    return;
}

void QueueSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}

