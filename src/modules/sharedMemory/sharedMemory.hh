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


class SharedMemory : public OneToOneFilter {

public:
    static SharedMemory* createNew(unsigned key_, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);
    ~SharedMemory();

    bool doProcessFrame(Frame *org, Frame *dst);
    FrameQueue* allocQueue(int wId);
    bool isEnabled() {return enabled;};

private:
    SharedMemory(unsigned key_ = KEY, size_t fTime = 0, FilterRole fRole_ = MASTER, bool force_ = false, bool sharedFrames_ = true);

    void initializeEventMap();
    void doGetState(Jzon::Object &filterNode);

    void copyOrgToDstFrame(InterleavedVideoFrame *org, InterleavedVideoFrame *dst);
    int writeSharedMemory(uint8_t *buffer, int buffer_size);
	uint8_t * readSharedMemory();
	void writeFramePayload(uint16_t seqNum);
	void readFramePayload();
	bool isWritable();
	bool isReadable();
	Frame * getFrameObject();
	bool setFrameObject(Frame* in_frame);
	uint16_t getSeqNum();

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
