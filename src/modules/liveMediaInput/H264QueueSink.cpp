#include "H264QueueSink.hh"
#include "../../Utils.hh"

static unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};

H264QueueSink::H264QueueSink(UsageEnvironment& env, unsigned port, char const* sPropParameterSetsStr)
: QueueSink(env, port),  fHaveWrittenFirstFrame(False), offset(0)
{
    fSPropParameterSetsStr = sPropParameterSetsStr;
}

H264QueueSink* H264QueueSink::createNew(UsageEnvironment& env,
                                        unsigned port, char const* sPropParameterSetsStr)
{
    return new H264QueueSink(env, port, sPropParameterSetsStr);
}

Boolean H264QueueSink::continuePlaying()
{
    if (fSource == NULL) {
        utils::errorMsg("Cannot play, fSource is null");
        return False;
    }

    if (!frame){
        utils::debugMsg("Using dummy buffer, no writer connected yet");
        fSource->getNextFrame(dummyBuffer, DUMMY_RECEIVE_BUFFER_SIZE,
                              QueueSink::afterGettingFrame, this,
                              QueueSink::onSourceClosure, this);
        return True;
    }
    
    if (nextFrame){
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
            (TaskFunc*)QueueSink::staticContinuePlaying, this);
        nextFrame = false;
        return True;
    }

    if (!fHaveWrittenFirstFrame) {
        utils::debugMsg("This is first frame, inject SPS and PPS from SDP if any");
        unsigned numSPropRecords;
        SPropRecord* sPropRecords
            = parseSPropParameterSets(fSPropParameterSetsStr, numSPropRecords);
        for (unsigned i = 0; i < numSPropRecords; ++i) {
            memmove(frame->getDataBuf() + offset, start_code, sizeof(start_code));
            memmove(frame->getDataBuf() + sizeof(start_code) + offset, sPropRecords[i].sPropBytes,
                    sPropRecords[i].sPropLength);
            offset += sizeof(start_code) + offset;
        }
        delete[] sPropRecords;
        fHaveWrittenFirstFrame = True;
    }

    memmove(frame->getDataBuf(), start_code, sizeof(start_code));

    fSource->getNextFrame(frame->getDataBuf() + sizeof(start_code) + offset,
                          frame->getMaxLength() - sizeof(start_code) - offset,
                          QueueSink::afterGettingFrame, this,
                          QueueSink::onSourceClosure, this);
    return True;
}

void H264QueueSink::afterGettingFrame(unsigned frameSize, struct timeval presentationTime)
{
    if (frame != NULL){
        frame->setLength(frameSize + sizeof(start_code) + offset);
        frame->newOriginTime();
        frame->setPresentationTime(std::chrono::system_clock::now());
        frame->setSequenceNumber(++seqNum);
        frame->setConsumed(true);
        offset = 0;
        nextFrame = true;
    }
    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
            (TaskFunc*)QueueSink::staticContinuePlaying, this);
}
