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

static unsigned char const start_code[4] = {0x00, 0x00, 0x00, 0x01};

SharedMemory* SharedMemory::createNew(size_t key_, VCodecType codec)
{
    SharedMemory *shm = new SharedMemory(key_, codec);

    if(shm->isEnabled()){
        return shm;
    }
    return NULL;
}

SharedMemory::SharedMemory(size_t key_, VCodecType codec_):
    OneToOneFilter(), enabled(true), newFrame(false), codec(codec_)
{

    if(!(codec == RAW || codec == H264)){
        utils::errorMsg("SharedMemory::error - filter not created - "
                "only RAW and H264 codecs are supported (requested " +
                utils::getVideoCodecAsString(codec_) + ")");
        enabled = false;
        return;
    }

    if ((SharedMemoryID = shmget(key_, SHMSIZE, (IPC_EXCL | IPC_CREAT ) | 0666)) == (size_t)-1) {
        utils::errorMsg("SharedMemory::shmget error - filter not created - "
                "might be already created (Key:" + std::to_string(key_) +
                " Codec: " + utils::getVideoCodecAsString(codec_) + ")");
        enabled = false;
        return;
    }

    if ((SharedMemoryOrigin = (uint8_t*) shmat(SharedMemoryID, NULL, 0)) == (uint8_t *) -1) {
        utils::errorMsg("SharedMemory::shmat error - filter not created");
        enabled = false;
        return;
    }

    if(enabled){
        utils::infoMsg("VERY IMPORTANT: Share following shared memory ID (from key "+ std::to_string(key_)+") with reader process: \033[1;32m"+ std::to_string(SharedMemoryID) + "\033[0m for \033[1;32m" + utils::getVideoCodecAsString(codec) + "\033[0m codec");

        memset(SharedMemoryOrigin,0,SHMSIZE);

        access = SharedMemoryOrigin;
        buffer = SharedMemoryOrigin + HEADER_SIZE;
        SharedMemorykey = key_;

        //init sync
        *access = CHAR_WRITING;

        //TODO get seqNum from incoming frame
        seqNum = 1;
    }
    
    fType = SHARED_MEMORY;

    streamInfo = NULL;
    maxFrames = 0;
}

SharedMemory::~SharedMemory()
{
    if(shmctl (SharedMemoryID , IPC_RMID , 0) != 0){
        utils::errorMsg("SharedMemory::shmctl error - Could not set IPC_RMID flag to shared memory segment ID");
    }
    if(shmdt(SharedMemoryOrigin) != 0){
        utils::errorMsg("SharedMemory::shmdt error - Could not detach memory segment");
    }
}

bool SharedMemory::doProcessFrame(Frame *org, Frame *dst)
{
    InterleavedVideoFrame* vframe = dynamic_cast<InterleavedVideoFrame*>(org);
    copyOrgToDstFrame(vframe,dynamic_cast<InterleavedVideoFrame*>(dst));

    if(!isWritable()){
        if(vframe->getCodec() == H264){
            if(vframe->getSequenceNumber() != seqNum && newFrame){
                seqNum = vframe->getSequenceNumber();
                frameData.clear();
            }
            parseNal(vframe, newFrame);
        }
        return true;
    }

    switch(vframe->getCodec()){
        case H264:
            if(vframe->getSequenceNumber() != seqNum && newFrame){
                writeFramePayload(vframe);
                writeSharedMemoryH264();
                seqNum = vframe->getSequenceNumber();
                frameData.clear();
            }
            parseNal(vframe, newFrame);
            break;
        case RAW:
            writeFramePayload(vframe);
            writeSharedMemoryRAW(vframe->getDataBuf(), vframe->getLength());
            break;
        default:
            utils::errorMsg("SharedMemory::error - only RAW and H264 frames are shareable");
            break;
    }

    return true;
}

FrameQueue* SharedMemory::allocQueue(ConnectionData cData)
{
    if (codec != H264 && codec !=RAW) {
        return NULL;
    }

    return VideoFrameQueue::createNew(cData, streamInfo, maxFrames);
}

//TODO to be implemented
void SharedMemory::initializeEventMap()
{

}

//TODO to be implemented
void SharedMemory::doGetState(Jzon::Object &/*filterNode*/)
{
   /* filterNode.Add("codec", utils::getAudioCodecAsString(fCodec));
    filterNode.Add("sampleRate", sampleRate);
    filterNode.Add("channels", channels);
    filterNode.Add("sampleFormat", utils::getSampleFormatAsString(sampleFmt));*/
}

void SharedMemory::copyOrgToDstFrame(InterleavedVideoFrame*org, InterleavedVideoFrame *dst)
{
    dst->setLength(org->getLength());
    dst->setSize(org->getWidth(), org->getHeight());
    dst->setPixelFormat(org->getPixelFormat());
    dst->setConsumed(true);
    dst->setPresentationTime(org->getPresentationTime());

    memcpy(dst->getDataBuf(), org->getDataBuf(),org->getLength());
}

void SharedMemory::writeSharedMemoryH264()
{
    uint8_t* data;
    unsigned dataLength;
    uint32_t length;
    data = reinterpret_cast<uint8_t*> (&frameData[0]);
    dataLength = frameData.size();
    length = dataLength;

    if (!data) {
        utils::errorMsg("SharedMemory::error - no data from appended frame");
        return;
    }

    *access = CHAR_WRITING;
    memcpy(access+20, &length, sizeof(uint32_t));
    memcpy(buffer, data, sizeof(uint8_t) * dataLength);
    *access = CHAR_READING;
}


bool SharedMemory::parseNal(VideoFrame* nal, bool &newFrame)
{
    int startCodeOffset;
    unsigned char* nalData;
    int nalDataLength;

    startCodeOffset = detectStartCode(nal->getDataBuf());

    if(startCodeOffset == 0){
        utils::errorMsg("Error parsing NAL: no start code detected");
        return false;
    }

    nalData = nal->getDataBuf();
    nalDataLength = nal->getLength();

    if (!nalData || nalDataLength <= 0) {
        utils::errorMsg("Error parsing NAL: invalid data pointer or length");
        return false;
    }

    if (!appendNalToFrame(nalData, nalDataLength, startCodeOffset, newFrame)) {
        utils::errorMsg("Error parsing NAL: invalid NALU type");
        return false;
    }

    return true;
}

int SharedMemory::detectStartCode(unsigned char const* ptr)
{
    u_int32_t bytes = 0|(ptr[0]<<16)|(ptr[1]<<8)|ptr[2];
    if (bytes == H264_NALU_START_CODE) {
        return SHORT_START_CODE_LENGTH;
    }

    bytes = (ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3];
    if (bytes == H264_NALU_START_CODE) {
        return LONG_START_CODE_LENGTH;
    }
    return 0;
}

bool SharedMemory::appendNalToFrame(unsigned char* nalData, unsigned nalDataLength, int startCodeOffset, bool &newFrame)
{
    unsigned char nalType;

    nalType = nalData[startCodeOffset] & H264_NALU_TYPE_MASK;

    switch (nalType) {
        case SPS:
            newFrame = false;
            break;
        case PPS:
            newFrame = false;
            break;
        case SEI:
            newFrame = false;
            break;
        case AUD:
            newFrame = true;
            break;
        case IDR:
            newFrame = false;
            break;
        case NON_IDR:
            newFrame = false;
            break;
        default:
            utils::errorMsg("Error parsing NAL: NalType " + std::to_string(nalType) + " not contemplated");
            return false;
    }

    if (nalType != AUD) {
        frameData.insert(frameData.end(), start_code, start_code + sizeof(start_code));
        frameData.insert(frameData.end(), nalData + startCodeOffset, nalData + nalDataLength);
    }

    return true;
}

int SharedMemory::writeSharedMemoryRAW(uint8_t *buf, int buf_size)
{

    *access = CHAR_WRITING;
    memcpy(buffer, buf, sizeof(uint8_t) * buf_size);
    *access = CHAR_READING;

    return 0;
}

void SharedMemory::writeFramePayload(InterleavedVideoFrame *frame) 
{
    uint32_t tv_sec = frame->getPresentationTime().count()/std::micro::den;
    uint32_t tv_usec = frame->getPresentationTime().count()%std::micro::den;
    uint16_t width = frame->getWidth();
    uint16_t height = frame->getHeight();
    uint32_t length = frame->getLength();
    uint16_t codec = getCodecFromVCodec(frame->getCodec());
    uint16_t pixFmt = getPixelFormatFromPixType(frame->getPixelFormat());
    uint16_t seqN = frame->getSequenceNumber();

    memcpy(access+2, &seqN, sizeof(uint16_t));
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
            utils::errorMsg("[Video Frame Queue] Codec not supported!");
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
            utils::errorMsg("[Video Frame Queue] Codec not supported!");
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
            utils::errorMsg("[Resampler] Unknown output pixel format");
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
            utils::errorMsg("[Resampler] Unknown output pixel format");
            pxlFrmt= P_NONE;
            break;
    }
    return pxlFrmt;
}

bool SharedMemory::specificReaderConfig(int /*readerID*/, FrameQueue* queue) {
    const AVFramedQueue *avQueue = dynamic_cast<AVFramedQueue*>(queue);
    if (avQueue == NULL) {
        utils::errorMsg("Input queue is not AV");
        return false;
    }
    maxFrames = avQueue->getMaxFrames();
    streamInfo = avQueue->getStreamInfo();
    return true;
}
