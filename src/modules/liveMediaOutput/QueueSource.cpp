#include "QueueSource.hh"
#include "SinkManager.hh"

QueueSource* QueueSource::createNew(UsageEnvironment& env, Reader *reader, int readerId) {
  return new QueueSource(env, reader, readerId);
}


QueueSource::QueueSource(UsageEnvironment& env, Reader *reader, int readerId)
  : FramedSource(env), fReader(reader), fReaderId(readerId) {
}

void QueueSource::doGetNextFrame() 
{
    checkStatus();
    bool newFrame = false;
    QueueState state;
    std::chrono::microseconds presentationTime;

    frame = fReader->getFrame(state, newFrame);

    if ((newFrame && frame == NULL) || (!newFrame && frame != NULL)) {
        //TODO: sanity check, think about assert
    }

    if (!newFrame) {
        nextTask() = envir().taskScheduler().scheduleDelayedTask(POLL_TIME,
            (TaskFunc*)QueueSource::staticDoGetNextFrame, this);
        return;
    }

    presentationTime = duration_cast<std::chrono::microseconds>(frame->getPresentationTime().time_since_epoch());

    fPresentationTime.tv_sec = presentationTime.count()/std::micro::den;
    fPresentationTime.tv_usec = presentationTime.count()%std::micro::den;

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

