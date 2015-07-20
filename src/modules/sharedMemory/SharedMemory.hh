/*
 *  SharedMemory - Sharing frames filter through shm
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
 *  Authors:    Gerard Castillo <gerard.castillo@i2cat.net>
 */

#ifndef _SHARED_MEMORY_HH
#define _SHARED_MEMORY_HH

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <fstream>

#include "../../FrameQueue.hh"
#include "../../Filter.hh"
#include "../../VideoFrame.hh"
#include "../../AVFramedQueue.hh"

#define SHMSIZE   		6220824		//1920x1080x3 + 24
#define HEADER_SIZE		24			//4B * 6 (sync.byte and frame info)
#define CHAR_READING 	'1'
#define CHAR_WRITING 	'0'
#define MAX_SIZE 1920*1080*3
#define KEY 1985
#define NON_IDR 1
#define IDR 5
#define SEI 6
#define SPS 7
#define PPS 8
#define AUD 9
#define H264_NALU_START_CODE 0x00000001
#define SHORT_START_CODE_LENGTH 3
#define LONG_START_CODE_LENGTH 4
#define H264_NALU_TYPE_MASK 0x1F

/*! OneToOneFilter sharing memory with another process. This filter uses shm
library, a POSIX shared memory library to share specific address spaces between
different processes.
*/
class SharedMemory : public OneToOneFilter {

public:
    /**
    * Creates new shared memory object
    * @param key_ value for defining the piece of address to share
    * @param VCodecType codec value defined in order to correlate type of shared frames with its shared memory space
    * @return SharedMemory object or NULL if any error while creating
    * @see OneToOneFilter to check the inherated input params
    */
    static SharedMemory* createNew(size_t key_, VCodecType codec);
    /**
    * Class destructor
    */
    ~SharedMemory();
    /**
    * Returns the shared memory ID of this filter
    * @return SharedMemoryID of its sharedMemory filter
    */
    size_t getSharedMemoryID() { return SharedMemoryID;};

protected:
    SharedMemory(size_t key_, VCodecType codec_);
    bool isEnabled() {return enabled;};
    void writeSharedMemoryH264();
    bool appendNalToFrame(unsigned char* nalData, unsigned nalDataLength, int startCodeOffset, bool &newFrame);
    bool parseNal(VideoFrame* nal, bool &newFrame);
    int detectStartCode(unsigned char const* ptr);
    int writeSharedMemoryRAW(uint8_t *buffer, int buffer_size);
    void writeFramePayload(InterleavedVideoFrame *frame);
    bool isWritable();
    uint16_t getSeqNum() { return seqNum;};
    void setSeqNum(uint16_t seqNum_) { seqNum = seqNum_;};
    bool isNewFrame() { return newFrame;};
    void setNewFrame(bool newFrame_) { newFrame = newFrame_;};

private:
    bool doProcessFrame(Frame *org, Frame *dst);
    void initializeEventMap();
    void doGetState(Jzon::Object &filterNode);
    FrameQueue* allocQueue(ConnectionData cData);

    void copyOrgToDstFrame(InterleavedVideoFrame *org, InterleavedVideoFrame *dst);

    uint16_t getCodecFromVCodec(VCodecType codec);
    uint16_t getPixelFormatFromPixType(PixType pxlFrmt);
    PixType getPixTypeFromPixelFormat(uint16_t pixType);
    VCodecType getVCodecFromCodecType(uint16_t codecType);

private:
    size_t      SharedMemorykey;
    size_t      SharedMemoryID;
    uint8_t     *SharedMemoryOrigin;
    uint8_t     *buffer;
    uint8_t     *access;
    bool        enabled;
    bool        newFrame;
    std::vector<unsigned char>    frameData;
    VCodecType const              codec;
    uint16_t                      seqNum;
};

#endif
