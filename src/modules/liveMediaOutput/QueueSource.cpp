#include "QueueSource.hh"
#include "SinkManager.hh"

QueueSource* QueueSource::createNew(UsageEnvironment& env, int readerId) 
{
  return new QueueSource(env, readerId);
}


QueueSource::QueueSource(UsageEnvironment& env, int readerId)
  : FramedSource(env), eventTriggerId(0), frame(NULL), fReaderId(readerId) 
{
    if (eventTriggerId == 0){
        eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
    }
}

void QueueSource::doGetNextFrame() 
{
    //TODO: check status
    if (false) {
        handleClosure();
        return;
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
    
    if (!frame) {
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
    frame->setConsumed(true);
    frame = NULL;

    afterGetting(this);
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

