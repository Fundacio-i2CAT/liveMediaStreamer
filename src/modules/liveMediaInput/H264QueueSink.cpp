#include "H264QueueSink.hh"
#include "../../Utils.hh"

static unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};

H264QueueSink::H264QueueSink(UsageEnvironment& env, Writer *writer, unsigned port, char const* sPropParameterSetsStr)
: QueueSink(env, writer, port),  fHaveWrittenFirstFrame(False)
{
    fSPropParameterSetsStr = sPropParameterSetsStr;
}

H264QueueSink* H264QueueSink::createNew(UsageEnvironment& env, Writer *writer,
                                        unsigned port, char const* sPropParameterSetsStr)
{
    return new H264QueueSink(env, writer, port, sPropParameterSetsStr);
}

Boolean H264QueueSink::continuePlaying()
{
    if (fSource == NULL) {
        utils::errorMsg("Cannot play, fSource is null");
        return False;
    }

    if (!fWriter->isConnected()){
        utils::debugMsg("Using dummy buffer, no writer connected yet");
        fSource->getNextFrame(dummyBuffer, DUMMY_RECEIVE_BUFFER_SIZE,
                              QueueSink::afterGettingFrame, this,
                              QueueSink::onSourceClosure, this);
        return True;
    }

    frame = fWriter->getFrame(true);

    if (!fHaveWrittenFirstFrame) {
        utils::debugMsg("This is first frame, inject SPS and PPS from SDP if any");
        unsigned numSPropRecords;
        SPropRecord* sPropRecords
            = parseSPropParameterSets(fSPropParameterSetsStr, numSPropRecords);
        for (unsigned i = 0; i < numSPropRecords; ++i) {
            memmove(frame->getDataBuf(), start_code, sizeof(start_code));
            memmove(frame->getDataBuf() + sizeof(start_code), sPropRecords[i].sPropBytes,
                    sPropRecords[i].sPropLength);
            frame->setLength(sPropRecords[i].sPropLength + sizeof(start_code));
            frame->newOriginTime();
            fWriter->addFrame();
            frame = fWriter->getFrame(true);
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
    if (frame != NULL){
        frame->setLength(frameSize + sizeof(start_code));
        frame->newOriginTime();
        frame->setPresentationTime(std::chrono::system_clock::now());
        frame->setSequenceNumber(++seqNum);
        frame->setConsumed(true);
        fWriter->addFrame();
    }
    continuePlaying();
}
