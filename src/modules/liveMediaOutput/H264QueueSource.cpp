/*
 *  H264QueueSource - A live 555 source which consumes frames from a LMS H264 queue
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors: David Cassany <david.cassany@i2cat.net> 
 *           Marc Palau <marc.palau@i2cat.net>
 */

#include "H264QueueSource.hh"
#include "../../VideoFrame.hh"
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
    std::chrono::microseconds presentationTime;

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

    presentationTime = duration_cast<std::chrono::microseconds>(frame->getPresentationTime().time_since_epoch());

    fPresentationTime.tv_sec = presentationTime.count()/std::micro::den;
    fPresentationTime.tv_usec = presentationTime.count()%std::micro::den;

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

