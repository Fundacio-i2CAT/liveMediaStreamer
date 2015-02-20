/*
 *  SharedMemoryDummyReader.cpp - Shared memory dummy reader and shareMemory filter mockup
 *  Copyright (C) 2015  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *
 */

#ifndef SHAREDMEMORYTESTUTILS_H_
#define SHAREDMEMORYTESTUTILS_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <iostream>
#include <unistd.h>

#include "../src/Frame.hh"
#include "../src/modules/sharedMemory/SharedMemory.hh"


class SharedMemoryFilterMockup : public SharedMemory
{
public:
	SharedMemoryFilterMockup(size_t key_);
	~SharedMemoryFilterMockup();
	using BaseFilter::getReader;
	using SharedMemory::getSharedMemoryID;
	using SharedMemory::isEnabled;
	using SharedMemory::writeSharedMemoryRAW;
	using SharedMemory::writeFramePayload;
	using SharedMemory::isWritable;
	using SharedMemory::getFrameObject;
	using SharedMemory::setFrameObject;
	using SharedMemory::getSeqNum;

private:
	size_t _key;

};

class SharedMemoryDummyReader {

public:
	SharedMemoryDummyReader(size_t shmID);
	~SharedMemoryDummyReader();
	uint8_t * readSharedFrame();
	void readFramePayload();
	bool isReadable();
	Frame * getFrameObject();
	bool setFrameObject(Frame* in_frame);
	uint16_t getSeqNum();
	void setDisabled() {enabled = false;};
	int writeFrameToFile(unsigned char const * const buff, size_t length, size_t seqNum);
    size_t getSharedMemoryID() { return SharedMemoryID;};
	bool isEnabled() {return enabled;};
	static void dummyReaderThread(SharedMemoryDummyReader* dummyReader);

    uint16_t getCodecFromVCodec(VCodecType codec);
	uint16_t getPixelFormatFromPixType(PixType pxlFrmt);
	PixType getPixTypeFromPixelFormat(uint16_t pixType);
	VCodecType getVCodecFromCodecType(uint16_t codecType);

private:
	size_t 						SharedMemoryID;
    uint8_t                     *SharedMemoryOrigin;
	uint8_t 					*buffer;
	uint8_t 					*access;
	InterleavedVideoFrame 		*frame;
	uint16_t					seqNum;
    bool                        enabled;

};

#endif /* SHAREDMEMORYTESTUTILS_H_ */
