#include "H264QueueSink.hh"
#include <iostream>

static unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};

H264QueueSink::H264QueueSink(UsageEnvironment& env, FrameQueue* queue, char const* sPropParameterSetsStr)
: QueueSink(env, queue),  fHaveWrittenFirstFrame(False)
{
    fSPropParameterSetsStr = sPropParameterSetsStr;
}

H264QueueSink* H264QueueSink::createNew(UsageEnvironment& env, FrameQueue* queue, 
                                        char const* sPropParameterSetsStr) 
{
    return new H264QueueSink(env, queue, sPropParameterSetsStr);
}

Boolean H264QueueSink::continuePlaying() 
{
    if (fSource == NULL) {
        return False;
    }
    
    updateFrame();
    
    if (!fHaveWrittenFirstFrame) {
        unsigned numSPropRecords;
        SPropRecord* sPropRecords
            = parseSPropParameterSets(fSPropParameterSetsStr, numSPropRecords);
        for (unsigned i = 0; i < numSPropRecords; ++i) {
            memmove(frame->getDataBuf(), start_code, sizeof(start_code));
            memmove(frame->getDataBuf() + sizeof(start_code), sPropRecords[i].sPropBytes,
                    sPropRecords[i].sPropLength);
            frame->setLength(sPropRecords[i].sPropLength + sizeof(start_code));
            frame->setUpdatedTime();
            queue->addFrame();
            updateFrame();
        }
        delete[] sPropRecords;
        fHaveWrittenFirstFrame = True;
    }
    
    memmove(frame->getDataBuf(), start_code, sizeof(start_code));
    
    fSource->getNextFrame(frame->getDataBuf() + sizeof(start_code), 
                          frame->getMaxLength() - sizeof(start_code),
                          QueueSink::afterGettingFrame, this,
                          QueueSink::onSourceClosure, this);
    
    return True;
}

void H264QueueSink::afterGettingFrame(unsigned frameSize, struct timeval presentationTime) 
{
    frame->setLength(frameSize + sizeof(start_code));
    frame->setUpdatedTime();
    frame->setPresentationTime(presentationTime);
    queue->addFrame();
    
    // Then try getting the next frame:
    continuePlaying();
}

