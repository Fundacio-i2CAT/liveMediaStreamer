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

#include "SharedMemoryDummyReader.hh"


SharedMemoryFilterMockup::SharedMemoryFilterMockup(size_t key_, VCodecType codec):
	SharedMemory(key_, codec)
{
}

SharedMemoryFilterMockup::~SharedMemoryFilterMockup()
{
}



SharedMemoryDummyReader::SharedMemoryDummyReader(size_t shmID, VCodecType codec):
	SharedMemoryID(shmID), frame(NULL), enabled (true)
{
    if ((SharedMemoryOrigin = (uint8_t*) shmat(SharedMemoryID, NULL, 0)) == (uint8_t *) -1) {
        utils::infoMsg("SharedMemory::shmat error - filter not created");
        enabled = false;
        return;
    }

    if(enabled){
        memset(SharedMemoryOrigin,0,SHMSIZE);

        access = SharedMemoryOrigin;
        buffer = SharedMemoryOrigin + HEADER_SIZE;

        //init sync
        *access = CHAR_WRITING;
    }
}

SharedMemoryDummyReader::~SharedMemoryDummyReader()
{
	enabled = false;
    if(shmdt(SharedMemoryOrigin) != 0){
        utils::infoMsg("SharedMemory::shmdt error - Could not detach memory segment");
    }
}

uint8_t * SharedMemoryDummyReader::readSharedFrame() {
	*access = CHAR_READING;
	memcpy(access + HEADER_SIZE, buffer, sizeof(uint8_t) * SHMSIZE);
	*access = CHAR_WRITING;

	return buffer;
}

void SharedMemoryDummyReader::readFramePayload() {

	uint32_t tv_sec;
	uint32_t tv_usec;
	uint16_t width;
	uint16_t height;
	uint32_t length;
	uint16_t codec;
	uint16_t pixFmt;

	memcpy(&seqNum, access+2, sizeof(uint16_t));
	memcpy(&pixFmt, access+4, sizeof(uint16_t));
	memcpy(&codec, access+6, sizeof(uint16_t));
	memcpy(&height, access+8, sizeof(uint16_t));
	memcpy(&width, access+10, sizeof(uint16_t));
	memcpy(&tv_sec, access+12, sizeof(uint32_t));
	memcpy(&tv_usec, access+16, sizeof(uint32_t));
	memcpy(&length, access+20, sizeof(uint32_t));

	if(!frame){
		utils::debugMsg("SharedMemoryDummyReader::Initializing Frame Object");
		frame = InterleavedVideoFrame::createNew(getVCodecFromCodecType(codec), width, height, getPixTypeFromPixelFormat(pixFmt));
	}

	frame->setSize(width, height);
	frame->setPixelFormat(getPixTypeFromPixelFormat(pixFmt));
	frame->setLength(length);
	frame->setPresentationTime(std::chrono::milliseconds(1000000 * tv_sec + tv_usec));

	utils::debugMsg("Reading Frame Payload: seqNum = " + std::to_string(seqNum) + " pixFmt = " + std::to_string(pixFmt) + " codec = "+ std::to_string(codec) + " size = " + std::to_string(width) + "x" + std::to_string(height) + " ts(sec) = " + std::to_string(tv_sec) + " ts(usec) = "+ std::to_string(tv_usec) + " length = " + std::to_string(length) +"");
}

bool SharedMemoryDummyReader::isReadable() {
	return *access == CHAR_READING;
}


Frame * SharedMemoryDummyReader::getFrameObject(){
	return frame;
}

bool SharedMemoryDummyReader::setFrameObject(Frame* in_frame){
	if((frame = dynamic_cast<InterleavedVideoFrame*>(in_frame))){
		return true;
	}else{
		frame = NULL;
		return false;
	}
}

uint16_t SharedMemoryDummyReader::getSeqNum(){
	return seqNum;
}

int SharedMemoryDummyReader::writeFrameToFile(unsigned char const * const buff, size_t length, size_t seqNum)
{
	std::string fileName = "frame" + std::to_string(seqNum);
	utils::debugMsg("Got frame: " + fileName );
	/*
	std::ofstream fs;
	fs.open (fileName, std::ostream::out | std::ofstream::app | std::ofstream::binary);

	if (fs.is_open()){
		fs.write(reinterpret_cast<char const * const>(buff), length);
	} else {
		return -1;
	}
	fs.close();
	*/
	return 0;
}

void SharedMemoryDummyReader::dummyReaderThread(SharedMemoryDummyReader* dummyReader){

    while(dummyReader->isEnabled()){
    	usleep(1000);
    	if(dummyReader->isReadable()){
    		dummyReader->readFramePayload();
			if(dummyReader->writeFrameToFile(dummyReader->readSharedFrame(), dummyReader->getFrameObject()->getLength(), dummyReader->getSeqNum()) < 0){
				utils::errorMsg("Error, could not open output file");
			}
    	}
    }
}

uint16_t SharedMemoryDummyReader::getCodecFromVCodec(VCodecType codec){
	uint16_t val = 0;
	switch(codec) {
		case H264:
			val= 1;
			break;
		case VP8:
			val= 2;
			break;
		case MJPEG:
			val= 3;
			break;
		case RAW:
			val= 4;
			break;
		default:
			std::cerr << "[Video Frame Queue] Codec not supported!" << std::endl;
			val= 0;
			break;
	}
	return val;
}

VCodecType SharedMemoryDummyReader::getVCodecFromCodecType(uint16_t codecType){
	VCodecType codec = VC_NONE;
	switch(codecType) {
		case 1:
			codec= H264;
			break;
		case 2:
			codec= VP8;
			break;
		case 3:
			codec= MJPEG;
			break;
		case 4:
			codec= RAW;
			break;
		default:
			std::cerr << "[Video Frame Queue] Codec not supported!" << std::endl;
			codec= VC_NONE;
			break;
	}
	return codec;
}

uint16_t SharedMemoryDummyReader::getPixelFormatFromPixType(PixType pxlFrmt){
	uint16_t val = 0;
	switch(pxlFrmt){
		case RGB24:
			val= 1;
			break;
		case RGB32:
			val= 2;
			break;
		case YUV420P:
			val= 3;
			break;
		case YUV422P:
			val= 4;
			break;
		case YUV444P:
			val= 5;
			break;
		case YUYV422:
			val= 6;
			break;
		case YUVJ420P:
			val= 7;
			break;
		default:
			std::cerr << "[Resampler] Unknown output pixel format" << std::endl;
			val= 0;
			break;
	}
	return val;
}

PixType SharedMemoryDummyReader::getPixTypeFromPixelFormat(uint16_t pixType){
	PixType pxlFrmt = P_NONE;
	switch(pixType){
		case 1:
			pxlFrmt = RGB24;
			break;
		case 2:
			pxlFrmt= RGB32;
			break;
		case 3:
			pxlFrmt= YUV420P;
			break;
		case 4:
			pxlFrmt= YUV422P;
			break;
		case 5:
			pxlFrmt= YUV444P;
			break;
		case 6:
			pxlFrmt= YUYV422;
			break;
		case 7:
			pxlFrmt= YUVJ420P;
			break;
		default:
			std::cerr << "[Resampler] Unknown output pixel format" << std::endl;
			pxlFrmt= P_NONE;
			break;
	}
	return pxlFrmt;
}
