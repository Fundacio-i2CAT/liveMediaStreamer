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

#include "../../FrameQueue.hh"
#include "../../Filter.hh"
#include "../../VideoFrame.hh"
#include "../../AVFramedQueue.hh"

#define SHMSIZE   		6220824		//1920x1080x3 + 24
#define HEADER_SIZE		24			//4B * 6 (sync.byte and frame info)
#define CHAR_READING 	'0'
#define CHAR_WRITING 	'1'
#define MAX_SIZE 1920*1080*3
#define KEY 6789

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
    static SharedMemory* createNew(unsigned key_, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);
    /**
    * Class destructor
    */
    ~SharedMemory();

    /**
    * Main method that bypass incoming frame to output and checks if the shared
    * memory can be written with new incoming frame
    * @param Frame *org as origin input frame
    * @param Frame *dst as destination output frame
    * @return true always, an incoming frame must be always bypassed
    */
    bool doProcessFrame(Frame *org, Frame *dst);
    /**
    * Allocs frame queue to be used
    * @param Integer of the worker ID
    * @return FrameQueue object or NULL if any error while creating the queue
    */
    FrameQueue* allocQueue(int wId);

private:
    SharedMemory(unsigned key_ = KEY, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);
    bool isEnabled() {return enabled;};

    void initializeEventMap();
    void doGetState(Jzon::Object &filterNode);

    void copyOrgToDstFrame(InterleavedVideoFrame *org, InterleavedVideoFrame *dst);
    int writeSharedMemory(uint8_t *buffer, int buffer_size);
	void writeFramePayload(uint16_t seqNum);
	bool isWritable();
	Frame * getFrameObject() { return frame;};
	bool setFrameObject(Frame* in_frame);
	uint16_t getSeqNum() { return seqNum;};

    uint16_t getCodecFromVCodec(VCodecType codec);
	uint16_t getPixelFormatFromPixType(PixType pxlFrmt);
	PixType getPixTypeFromPixelFormat(uint16_t pixType);
	VCodecType getVCodecFromCodecType(uint16_t codecType);

private:
	key_t 						SharedMemorykey;
	int 						SharedMemoryID;
    uint8_t                     *SharedMemoryOrigin;
	uint8_t 					*buffer;
	uint8_t 					*access;
	InterleavedVideoFrame 		*frame;
	uint16_t 					seqNum;
    bool                        enabled;
};

#endif
