#include "QueueSource.hh"
#include "SinkManager.hh"

QueueSource* QueueSource::createNew(UsageEnvironment& env, Reader *reader, int readerId) {
  return new QueueSource(env, reader, readerId);
}


QueueSource::QueueSource(UsageEnvironment& env, Reader *reader, int readerId)
  : FramedSource(env), fReader(reader), fReaderId(readerId) {
}

void QueueSource::doGetNextFrame() 
{//TODO: fDurationInMicroseconds
    checkStatus();

    if ((frame = fReader->getFrame()) == NULL) {
        nextTask() = envir().taskScheduler().scheduleDelayedTask(POLL_TIME,
            (TaskFunc*)QueueSource::staticDoGetNextFrame, this);
        return;
    }
    
    fPresentationTime.tv_sec = frame->getPresentationTime().count()/1000000;
    fPresentationTime.tv_usec = frame->getPresentationTime().count()%1000000;

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

void QueueSource::staticDoGetNextFrame(FramedSource* source) {
    source->doGetNextFrame();
}

void QueueSource::checkStatus()
{
    if (fReader->isConnected()) {
        return;
    }

    stopGettingFrames();
}

