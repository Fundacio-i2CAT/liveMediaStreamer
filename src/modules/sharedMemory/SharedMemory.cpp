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

#include "SharedMemory.hh"

SharedMemory* SharedMemory::createNew(size_t key_, size_t fTime, FilterRole fRole_, bool force_, bool sharedFrames_)
{
    SharedMemory *shm = new SharedMemory(key_, fTime, fRole_, force_, sharedFrames_);

    if(shm->isEnabled()){
        return shm;
    }
    return NULL;
}

SharedMemory::SharedMemory(size_t key_, size_t fTime, FilterRole fRole_, bool force_, bool sharedFrames_):
    OneToOneFilter(fTime, fRole_), enabled(true)
{
    if ((SharedMemoryID = shmget(key_, SHMSIZE, (IPC_EXCL | IPC_CREAT ) | 0666)) < 0) {
        utils::debugMsg("SharedMemory::shmget error - filter not created - might be already created");
        enabled = false;
        return;
    }

    if ((SharedMemoryOrigin = (uint8_t*) shmat(SharedMemoryID, NULL, 0)) == (uint8_t *) -1) {
        utils::debugMsg("SharedMemory::shmat error - filter not created");
        enabled = false;
        return;
    }

    if(enabled){
        utils::infoMsg("VERY IMPORTANT: Share following shared memory key ID with reader process: \033[1;32m"+ std::to_string(SharedMemoryID) + "\033[0m");

        memset(SharedMemoryOrigin,0,SHMSIZE);

        access = SharedMemoryOrigin;
        buffer = SharedMemoryOrigin + HEADER_SIZE;
        SharedMemorykey = key_;

        //init sync
        *access = CHAR_WRITING;

        //TODO get seqNum from incoming frame
        seqNum = 0;
    }
}

SharedMemory::~SharedMemory()
{
    if(shmctl (SharedMemoryID , IPC_RMID , 0) != 0){
        utils::infoMsg("SharedMemory::shmctl error - Could not set IPC_RMID flag to shared memory segment ID");
    }
    if(shmdt(SharedMemoryOrigin) != 0){
        utils::infoMsg("SharedMemory::shmdt error - Could not detach memory segment");
    }
}

bool SharedMemory::doProcessFrame(Frame *org, Frame *dst)
{
    //TODO get seqNum from incoming frame
    seqNum++;

    InterleavedVideoFrame* vframe = dynamic_cast<InterleavedVideoFrame*>(org);
    copyOrgToDstFrame(vframe,dynamic_cast<InterleavedVideoFrame*>(dst));

    if(!isWritable()){
        return true;
    }

    if(!setFrameObject(vframe)){
        return true;
    }

    writeFramePayload(seqNum);
    writeSharedMemory(getFrameObject()->getDataBuf(), getFrameObject()->getLength());

    return true;
}

FrameQueue* SharedMemory::allocQueue(int wId)
{
    return VideoFrameQueue::createNew(RAW, RGB24);
}

//TODO to be implemented
void SharedMemory::initializeEventMap()
{

}

//TODO to be implemented
void SharedMemory::doGetState(Jzon::Object &filterNode)
{
   /* filterNode.Add("codec", utils::getAudioCodecAsString(fCodec));
    filterNode.Add("sampleRate", sampleRate);
    filterNode.Add("channels", channels);
    filterNode.Add("sampleFormat", utils::getSampleFormatAsString(sampleFmt));*/
}

void SharedMemory::copyOrgToDstFrame(InterleavedVideoFrame*org, InterleavedVideoFrame *dst){
    dst->setLength(org->getLength());
    dst->setSize(org->getWidth(), org->getHeight());
    dst->setPresentationTime(org->getPresentationTime());
    dst->setOriginTime(org->getOriginTime());
    dst->setPixelFormat(org->getPixelFormat());
    memcpy(dst->getDataBuf(), org->getDataBuf(),org->getLength());
}

int SharedMemory::writeSharedMemory(uint8_t *buf, int buf_size) {

	*access = CHAR_WRITING;
	memcpy(buffer, buf, sizeof(uint8_t) * buf_size);
	*access = CHAR_READING;

	return 0;
}

void SharedMemory::writeFramePayload(uint16_t seqNum) {

	uint32_t tv_sec = frame->getPresentationTime().count() / 1000000;
	uint32_t tv_usec = frame->getPresentationTime().count() % 1000000;
	uint16_t width = frame->getWidth();
	uint16_t height = frame->getHeight();
	uint32_t length = frame->getLength();
	uint16_t codec = getCodecFromVCodec(frame->getCodec());
	uint16_t pixFmt = getPixelFormatFromPixType(frame->getPixelFormat());

    utils::debugMsg("Writing Frame Payload: seqNum = " + std::to_string(seqNum) + " pixFmt = " + std::to_string(pixFmt) + " codec = "+ std::to_string(codec) + " size = " + std::to_string(width) + "x" + std::to_string(height) + " ts(sec) = " + std::to_string(tv_sec) + " ts(usec) = "+ std::to_string(tv_usec) + " length = " + std::to_string(length) +"");

	memcpy(access+2, &seqNum, sizeof(uint16_t));
	memcpy(access+4, &pixFmt, sizeof(uint16_t));
	memcpy(access+6, &codec, sizeof(uint16_t));
	memcpy(access+8, &height, sizeof(uint16_t));
	memcpy(access+10, &width, sizeof(uint16_t));
	memcpy(access+12, &tv_sec, sizeof(uint32_t));
	memcpy(access+16, &tv_usec, sizeof(uint32_t));
	memcpy(access+20, &length, sizeof(uint32_t));
}

bool SharedMemory::isWritable() {
	return *access == CHAR_WRITING;
}

bool SharedMemory::setFrameObject(Frame* in_frame){
	if((frame = dynamic_cast<InterleavedVideoFrame*>(in_frame))){
		return true;
	}else{
		frame = NULL;
		return false;
	}
}

uint16_t SharedMemory::getCodecFromVCodec(VCodecType codec){
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

VCodecType SharedMemory::getVCodecFromCodecType(uint16_t codecType){
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

uint16_t SharedMemory::getPixelFormatFromPixType(PixType pxlFrmt){
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

PixType SharedMemory::getPixTypeFromPixelFormat(uint16_t pixType){
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
