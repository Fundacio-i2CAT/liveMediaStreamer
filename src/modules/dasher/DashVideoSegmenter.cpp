/*
 *  DashVideoSegmenter.cpp - DASH video stream segmenter
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 *            Gerard Castillo <gerard.castillo@i2cat.net>
 *            David Cassany <david.cassany@i2cat.net>  
 *
 */

 #include "DashVideoSegmenter.hh"

DashVideoSegmenter::DashVideoSegmenter(std::chrono::seconds segDur, std::string video_format_, std::chrono::microseconds& offset) : 
DashSegmenter(segDur, DASH_VIDEO_TIME_BASE, offset), 
currentIntra(false), previousIntra(false), video_format(video_format_)
{

}

DashVideoSegmenter::~DashVideoSegmenter()
{

}

Frame* DashVideoSegmenter::manageFrame(Frame* frame)
{
    VideoFrame* nal;
    VideoFrame* vFrame;

    nal = dynamic_cast<VideoFrame*>(frame);

    if (!nal) {
        utils::errorMsg("Error managing frame: it MUST be a video frame");
        return NULL;
    }

    vFrame = parseNal(nal);

    updateExtradata();
    
    if (!vFrame || vFrame->getLength() == 0) {
        return NULL;
    }

    if(!setup(vFrame->getWidth(), vFrame->getHeight())) {
        utils::errorMsg("Error during Dash Video Segmenter setup");
        return NULL;
    }

    return vFrame;
}

bool DashVideoSegmenter::setup(size_t width, size_t height)
{
    uint8_t i2error = I2OK;

    if (!dashContext) {
        i2error = generateContext();
    }

    if (i2error != I2OK || !dashContext) {
        return false;
    }

    i2error = fill_video_context(&dashContext, width, height, timeBase);

    if (i2error != I2OK) {
        return false;
    }

    set_segment_duration(segDurInTimeBaseUnits, &dashContext);
    return true;
}

bool DashVideoSegmenter::generateInitSegment(DashSegment* segment) 
{
    size_t initSize = 0;
    unsigned char* data;
    unsigned dataLength;

    if (!dashContext || extradata.empty() || !segment || !segment->getDataBuffer()) {
        return false;
    }

    data = reinterpret_cast<unsigned char*> (&extradata[0]);
    dataLength = extradata.size();

    if (!data) {
        return false;
    }

    initSize = new_init_video_handler(data, dataLength, segment->getDataBuffer(), &dashContext);

    if (initSize < I2ERROR_MAX) {
        return false;
    }

    segment->setDataLength(initSize);
    extradata.clear();
    return true;
}

unsigned DashVideoSegmenter::customGenerateSegment(unsigned char *segBuffer, std::chrono::microseconds nextFrameTs, 
                                                    unsigned &segTimestamp, unsigned &segDuration, bool force)
{
    size_t timeBasePts;

    timeBasePts = microsToTimeBase(nextFrameTs);

    return generate_video_segment(isIntraFrame(), timeBasePts, segBuffer, &dashContext, &segTimestamp, &segDuration);
}

bool DashVideoSegmenter::appendFrameToDashSegment(DashSegment* segment, Frame* frame)
{
    size_t addSampleReturn;
    size_t timeBasePts;

    if (!frame || !frame->getDataBuf() || frame->getLength() <= 0 || !dashContext) {
        utils::errorMsg("Error appeding frame to segment: frame not valid");
        return false;
    }

    timeBasePts = microsToTimeBase(frame->getPresentationTime());

    addSampleReturn = add_video_sample(frame->getDataBuf(), frame->getLength(), timeBasePts, 
                                        timeBasePts, segment->getSeqNumber(), isIntraFrame(), &dashContext);

    if (addSampleReturn != I2OK) {
        utils::errorMsg("Error adding video sample. Code error: " + std::to_string(addSampleReturn));
        return false;
    }

    return true;
}

bool DashVideoSegmenter::appendNalToFrame(VideoFrame* frame, unsigned char* nalData, unsigned nalDataLength, 
                                           unsigned nalWidth, unsigned nalHeight, std::chrono::microseconds ts)
{
    if (frame->getLength() + nalDataLength + AVCC_HEADER_BYTES_MINUS_ONE + 1 > frame->getMaxLength()) {
        utils::errorMsg("[DashVideoSegmenter::appendNalToFrame] Nal exceeds frame max length");
        return false;
    }

    frame->getDataBuf()[frame->getLength()] = (nalDataLength >> 24) & 0xFF;
    frame->getDataBuf()[frame->getLength()+1] = (nalDataLength >> 16) & 0xFF;
    frame->getDataBuf()[frame->getLength()+2] = (nalDataLength >> 8) & 0xFF;
    frame->getDataBuf()[frame->getLength()+3] = nalDataLength & 0xFF;
    frame->setLength(frame->getLength() + 4);

    memcpy(frame->getDataBuf() + frame->getLength(), nalData, nalDataLength);
    frame->setLength(frame->getLength() + nalDataLength);
    
    frame->setSize(nalWidth, nalHeight);
    frame->setPresentationTime(ts);
    return true;
}


int DashVideoSegmenter::detectStartCode(unsigned char const* ptr)
{
    u_int32_t bytes = 0|(ptr[0]<<16)|(ptr[1]<<8)|ptr[2];
    if (bytes == H264or5_NALU_START_CODE) {
        return SHORT_START_CODE_LENGTH;
    }

    bytes = (ptr[0]<<24)|(ptr[1]<<16)|(ptr[2]<<8)|ptr[3];
    if (bytes == H264or5_NALU_START_CODE) {
        return LONG_START_CODE_LENGTH;
    }
    return 0;
}

size_t DashVideoSegmenter::getWidth()
{
    if (!dashContext || !dashContext->ctxvideo) {
        return 0;
    }

    return dashContext->ctxvideo->width;

}

size_t DashVideoSegmenter::getHeight()
{
    if (!dashContext || !dashContext->ctxvideo) {
        return 0;
    }

    return dashContext->ctxvideo->height;
}
