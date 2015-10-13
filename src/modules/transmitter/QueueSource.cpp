#include "QueueSource.hh"
#include "SinkManager.hh"

QueueSource* QueueSource::createNew(UsageEnvironment& env, const StreamInfo* streamInfo) 
{
  return new QueueSource(env, streamInfo);
}


QueueSource::QueueSource(UsageEnvironment& env, const StreamInfo* streamInfo)
  : FramedSource(env), eventTriggerId(0), frame(NULL), si(streamInfo), processedFrame(false), stopFrames(true)
{
    if (eventTriggerId == 0){
        eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    }
}

void QueueSource::doGetNextFrame() 
{
    //TODO: check status, i.e. client disconnected!
    if (false) {
        handleClosure();
        return;
    }
    
    if (stopFrames){
        stopFrames = false; 
    }
    
    if (frame) {
        deliverFrame();
    }    
}

void QueueSource::deliverFrame0(void* clientData) {
  ((QueueSource*)clientData)->deliverFrame();
}

void QueueSource::deliverFrame()
{
    if (!isCurrentlyAwaitingData()) {
        return; 
    }
    
    if (!frame || processedFrame) {
        return;
    }

    fPresentationTime.tv_sec = frame->getPresentationTime().count()/std::micro::den;
    fPresentationTime.tv_usec = frame->getPresentationTime().count()%std::micro::den;

    if (fMaxSize < frame->getLength()){
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = frame->getLength() - fMaxSize;
    } else {
        fNumTruncatedBytes = 0; 
        fFrameSize = frame->getLength();
    }
    
    memcpy(fTo, frame->getDataBuf(), fFrameSize);
    processedFrame = true;
    
    afterGetting(this);
}

bool QueueSource::gotFrame()
{   
    if (processedFrame || stopFrames){
        processedFrame = false;
        if (frame){
            frame = NULL;
            return true;
        }
    }
    return false;
}

bool QueueSource::setFrame(Frame *f)
{
    if (f && !frame){
        frame = f;
        return true;
    }
    return false;
}

bool QueueSource::signalNewFrameData(TaskScheduler* ourScheduler, QueueSource* ourSource) 
{
    if (!ourScheduler || !ourSource) {
        return false;
    }

    ourScheduler->triggerEvent(ourSource->getTriggerId(), ourSource);
    return true;
}

void QueueSource::doStopGettingFrames()
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    stopFrames = true;
}

