#include "H264QueueSource.hh"
#include <iostream>

#define START_CODE 0x00000001
#define LENGTH_START_CODE 4

H264QueueSource* H264QueueSource::createNew(UsageEnvironment& env, Reader *reader) {
  return new H264QueueSource(env, reader);
}

H264QueueSource::H264QueueSource(UsageEnvironment& env, Reader *reader)
: QueueSource(env, reader) {
}

bool testStartCode(unsigned char const* ptr) {
    u_int32_t bytes = (ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3];
    return bytes == START_CODE;
}

void H264QueueSource::doGetNextFrame() {
    unsigned char* buff;
    int size;
    
    if ((frame = fReader->getFrame()) == NULL) {
        nextTask() = envir().taskScheduler().scheduleDelayedTask(1000,
            (TaskFunc*)QueueSource::staticDoGetNextFrame, this);
        return;
    }

    if (testStartCode(frame->getDataBuf())) {
        buff = frame->getDataBuf() + LENGTH_START_CODE;
        size = frame->getLength() - LENGTH_START_CODE;
    } else {
        buff = frame->getDataBuf();
        size = frame->getLength();
    }
    
    fPresentationTime = frame->getPresentationTime();
    
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

