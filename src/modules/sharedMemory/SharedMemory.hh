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
#define KEY 1907

/*! OneToOneFilter sharing memory with another process. This filter uses shm
library, a POSIX shared memory library to share specific address spaces between
different processes.
*/
class SharedMemory : public OneToOneFilter {

public:
    /**
    * Creates new shared memory object
    * @param key_ value for defining the piece of address to share
    * @return SharedMemory object or NULL if any error while creating
    * @see OneToOneFilter to check the inherated input params
    */
    static SharedMemory* createNew(size_t key_, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);
    /**
    * Class destructor
    */
    ~SharedMemory();
    /**
    * Returns the shared memory ID of this filter
    * @return SharedMemoryID of its sharedMemory filter
    */

protected:
    SharedMemory(size_t key_ = KEY, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);
    size_t getSharedMemoryID() { return SharedMemoryID;};
    bool isEnabled() {return enabled;};
    int writeSharedMemory(uint8_t *buffer, int buffer_size);
	void writeFramePayload(uint16_t seqNum);
	bool isWritable();
	Frame * getFrameObject() { return frame;};
	bool setFrameObject(Frame* in_frame);
	uint16_t getSeqNum() { return seqNum;};

protected:
	InterleavedVideoFrame 		*frame;

private:
    bool doProcessFrame(Frame *org, Frame *dst);
    void initializeEventMap();
    void doGetState(Jzon::Object &filterNode);
    FrameQueue* allocQueue(int wId);

    void copyOrgToDstFrame(InterleavedVideoFrame *org, InterleavedVideoFrame *dst);

    uint16_t getCodecFromVCodec(VCodecType codec);
	uint16_t getPixelFormatFromPixType(PixType pxlFrmt);
	PixType getPixTypeFromPixelFormat(uint16_t pixType);
	VCodecType getVCodecFromCodecType(uint16_t codecType);

private:
	size_t 						SharedMemorykey;
	size_t 						SharedMemoryID;
    uint8_t                     *SharedMemoryOrigin;
	uint8_t 					*buffer;
	uint8_t 					*access;
	uint16_t 					seqNum;
    bool                        enabled;
};

#endif
