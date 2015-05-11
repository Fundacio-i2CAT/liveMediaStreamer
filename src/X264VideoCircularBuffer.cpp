/*
 *  X264VideoCircularBuffer - X264 Video circular buffer
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
 *  Authors:  Martin German <martin.german@i2cat.net>
 *            David Cassany <david.cassany@i2cat.net>
 *            Marc Palau <marc.palau@i2cat.net>
 */

#include "X264VideoCircularBuffer.hh"
#include "Utils.hh"
#include <cstring>

X264VideoCircularBuffer* X264VideoCircularBuffer::createNew(unsigned maxFrames)
{
    X264VideoCircularBuffer* b;

    if (maxFrames <= 0) {
        return NULL;
    }

    b = new X264VideoCircularBuffer(maxFrames);

    if (!b->setup()) {
        utils::errorMsg("X264VideoCircularBuffer setup error");
        delete b;
        return NULL;
    }

    return b;
}


X264VideoCircularBuffer::X264VideoCircularBuffer(unsigned maxFrames) : X264or5VideoCircularBuffer(H264, maxFrames)
{
    inputFrame = X264VideoFrame::createNew();
}

X264VideoCircularBuffer::~X264VideoCircularBuffer()
{
    delete inputFrame;
}

bool X264VideoCircularBuffer::pushBack() 
{
    Frame* frame;
    VideoFrame* vFrame;
    int nalsNum;
    x264_nal_t* nals;
    X264VideoFrame* x264Frame;

    x264Frame = dynamic_cast<X264VideoFrame*>(inputFrame);

    nalsNum = x264Frame->getHdrNalsNum();
    nals = x264Frame->getHdrNals();

    for (int i=0; i<nalsNum; i++) {

        if ((frame = innerGetRear()) == NULL){
            frame = innerForceGetRear();
        }
        
        vFrame = dynamic_cast<VideoFrame*>(frame);
        vFrame->setSequenceNumber(x264Frame->getSequenceNumber());

        memcpy(vFrame->getDataBuf(), nals[i].p_payload, nals[i].i_payload);
        vFrame->setLength(nals[i].i_payload);
        vFrame->setPresentationTime(x264Frame->getPresentationTime());
        vFrame->setSize(x264Frame->getWidth(), x264Frame->getHeight());
        vFrame->setDuration(x264Frame->getDuration());
        innerAddFrame();
    }

    nalsNum = x264Frame->getNalsNum();
    nals = x264Frame->getNals();

    for (int i=0; i<nalsNum; i++) {

        if ((frame = innerGetRear()) == NULL){
            frame = innerForceGetRear();
        }

        vFrame = dynamic_cast<VideoFrame*>(frame);
        vFrame->setSequenceNumber(x264Frame->getSequenceNumber());

        memcpy(vFrame->getDataBuf(), nals[i].p_payload, nals[i].i_payload);
        vFrame->setLength(nals[i].i_payload);
        vFrame->setPresentationTime(x264Frame->getPresentationTime());
        vFrame->setSize(x264Frame->getWidth(), x264Frame->getHeight());
        vFrame->setDuration(x264Frame->getDuration());
		innerAddFrame();
	}
	
	x264Frame->clearNalNum();
	
    return true;
}   