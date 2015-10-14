/*
 *  H264or5QueueSource - A live 555 source which consumes frames from a LMS H264 queue
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

#include "H264or5QueueSource.hh"
#include "../../VideoFrame.hh"
#include "../../Utils.hh"
#include <stdio.h>

#define START_CODE 0x00000001
#define SHORT_START_LENGTH 3
#define LONG_START_LENGTH 4

#define H264_NALU_TYPE_MASK 0x1F
#define H265_NALU_TYPE_MASK 0x7E

#define VPS 32
#define SPS_HEVC 33
#define PPS_HEVC 34
#define SPS_AVC 7
#define PPS_AVC 8

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

H264or5QueueSource* H264or5QueueSource::createNew(UsageEnvironment& env, const StreamInfo *streamInfo) 
{
  return new H264or5QueueSource(env, streamInfo);
}

H264or5QueueSource::H264or5QueueSource(UsageEnvironment& env, const StreamInfo *streamInfo)
: QueueSource(env, streamInfo), fVPS(NULL), fSPS(NULL), fPPS(NULL), fVPSSize(0), fSPSSize(0), fPPSSize(0) 
{    
    if (si->video.h264or5.annexb && si->extradata_size > 0){
        parseExtradata();
    }
}

H264or5QueueSource::~H264or5QueueSource()
{
    if(fVPS){
        delete fVPS;
    }
    
    if (fSPS){
        delete fSPS;
    }
    
    if (fPPS){
        delete fPPS;
    }
}

bool H264or5QueueSource::parseExtradata()
{
    int offset = 0;
    int nalStart = 0;
    uint8_t *nal;
    
    if (!si->video.h264or5.annexb || si->extradata_size == 0){
        return false;
    }
    
    if ((offset = startOffset(si->extradata)) > 0){
        nalStart = offset;
        while (offset < si->extradata_size){
            if (startOffset(si->extradata + offset) > 0){
                nal = new uint8_t[offset - nalStart];
                memcpy(nal, si->extradata + nalStart, offset - nalStart);
                feedHeaders(nal, offset - nalStart, si->video.codec);
                offset += startOffset(si->extradata + offset);
                nalStart = offset;
            } else {
                offset++;
            }
        }
        nal = new uint8_t[si->extradata_size - nalStart];
        memcpy(nal, si->extradata + nalStart, si->extradata_size - nalStart);
        feedHeaders(nal, si->extradata_size - nalStart, si->video.codec);
    }
    
    if (si->video.codec == H264 && fSPS && fPPS){
        return true;
    }
    
    if (si->video.codec == H265 && fSPS && fPPS && fVPS){
        return true;
    }
    
    return false;
}

void H264or5QueueSource::feedHeaders(uint8_t *nal, uint8_t nalSize, VCodecType codec)
{
    uint8_t nalType;
    
    switch (codec){
        case H264:
            nalType = nal[0] & H264_NALU_TYPE_MASK;
            if (nalType == SPS_AVC) {
                fSPS = nal;
                fSPSSize = nalSize;
            } else if (nalType == PPS_AVC) {
                fPPS = nal;
                fPPSSize = nalSize;
            }
            break;
        case H265:
            nalType = (nal[0] & H265_NALU_TYPE_MASK) >> 1;
            if (nalType == SPS_HEVC) {
                fSPS = nal;
                fSPSSize = nalSize;
            } else if (nalType == PPS_HEVC) {
                fPPS = nal;
                fPPSSize = nalSize;
            } else if (nalType == VPS) {
                fVPS = nal;
                fVPSSize = nalSize;
            }
            break;
        default:
            break;
    }
}

void H264or5QueueSource::deliverFrame()
{
    if (!isCurrentlyAwaitingData()) {
        return; 
    }
    
    if (!frame) {
        return;
    }
    
    unsigned char* buff;
    unsigned int size;
    uint8_t offset;
    
    size = frame->getLength();
    buff = frame->getDataBuf();

    offset = startOffset(buff);
    
    buff = frame->getDataBuf() + offset;
    size = size - offset;

    fPresentationTime.tv_sec = frame->getPresentationTime().count()/std::micro::den;
    fPresentationTime.tv_usec = frame->getPresentationTime().count()%std::micro::den;

    if (fMaxSize < size){
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = size - fMaxSize;
    } else {
        fNumTruncatedBytes = 0; 
        fFrameSize = size;
    }
    
    memcpy(fTo, buff, fFrameSize);
    processedFrame = true;
    
    afterGetting(this);
}

