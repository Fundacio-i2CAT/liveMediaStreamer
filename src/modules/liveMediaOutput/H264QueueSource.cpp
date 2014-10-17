#include "H264QueueSource.hh"
#include "../../Utils.hh"
#include <stdio.h>

#define START_CODE 0x00000001
#define SHORT_START_LENGTH 3
#define LONG_START_LENGTH 4


H264QueueSource* H264QueueSource::createNew(UsageEnvironment& env, Reader *reader, int readerId) 
{
  return new H264QueueSource(env, reader, readerId);
}

H264QueueSource::H264QueueSource(UsageEnvironment& env, Reader *reader, int readerId)
: QueueSource(env, reader, readerId) {
}

uint8_t startOffset(unsigned char const* ptr) {
    u_int32_t bytes = 0|(ptr[0]<<16)|(ptr[1]<<8)|ptr[2];
    if (bytes == START_CODE) {
        return SHORT_START_LENGTH;
    }
    bytes = (ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3];
    if (bytes == START_CODE) {
        return LONG_START_LENGTH;
    }
    return 0;
}

void H264QueueSource::doGetNextFrame() {
    unsigned char* buff;
    unsigned int size;
    uint8_t offset;

    bool newFrame = false;
    QueueState state;

    frame = fReader->getFrame(state, newFrame);

    if ((newFrame && frame == NULL) || (!newFrame && frame != NULL)) {
        //TODO: sanity check, think about assert
    }

    if (!newFrame) {
        nextTask() = envir().taskScheduler().scheduleDelayedTask(POLL_TIME,
            (TaskFunc*)QueueSource::staticDoGetNextFrame, this);
        return;
    }
    
    size = frame->getLength();
    buff = frame->getDataBuf();
    
    offset = startOffset(buff);
    
    buff = frame->getDataBuf() + offset;
    size = size - offset;

    fPresentationTime.tv_sec = frame->getPresentationTime().count()/1000000;
    fPresentationTime.tv_usec = frame->getPresentationTime().count()%1000000;

    if (fMaxSize < size){
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = size - fMaxSize;
    } else {
        fNumTruncatedBytes = 0; 
        fFrameSize = size;
    }
    
    memcpy(fTo, buff, fFrameSize);
    fReader->removeFrame();
    afterGetting(this);
}

