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
 *
 */

 #include "DashVideoSegmenter.hh"

DashVideoSegmenter::DashVideoSegmenter(std::chrono::seconds segDur, std::string video_format_) : 
DashSegmenter(segDur, DASH_VIDEO_TIME_BASE), 
isIntra(false), video_format(video_format_)
{

}

DashVideoSegmenter::~DashVideoSegmenter()
{

}

bool DashVideoSegmenter::manageFrame(Frame* frame, bool &newFrame)
{
    VideoFrame* nal;

    nal = dynamic_cast<VideoFrame*>(frame);

    if (!nal) {
        utils::errorMsg("Error managing frame: it MUST be a video frame");
        return false;
    }

    if (!parseNal(nal, newFrame)) {
        utils::errorMsg("Error parsing NAL");
        return false;
    }

    if (newFrame && !setup(vFrame->getWidth(), vFrame->getHeight())) {
        utils::errorMsg("Error during Dash Audio Segmenter setup");
        return false;
    }

    return true;
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

bool DashVideoSegmenter::generateInitData(DashSegment* segment) 
{
    size_t initSize = 0;
    unsigned char* data;
    unsigned dataLength;

    if (!dashContext || metadata.empty() || !segment || !segment->getDataBuffer()) {
        return false;
    }

    data = reinterpret_cast<unsigned char*> (&metadata[0]);
    dataLength = metadata.size();

    if (!data) {
        return false;
    }

    initSize = new_init_video_handler(data, dataLength, segment->getDataBuffer(), &dashContext);

    if (initSize == 0) {
        return false;
    }

    segment->setDataLength(initSize);

    return true;
}

unsigned DashVideoSegmenter::customGenerateSegment(unsigned char *segBuffer, unsigned &segTimestamp, unsigned &segDuration, bool force)
{
    size_t timeBasePts;

    timeBasePts = microsToTimeBase(vFrame->getPresentationTime());

    return generate_video_segment(isIntra, timeBasePts, segBuffer, &dashContext, &segTimestamp, &segDuration);
}

bool DashVideoSegmenter::appendFrameToDashSegment(DashSegment* segment)
{
    size_t addSampleReturn;
    size_t timeBasePts;

    if (!vFrame->getDataBuf() || vFrame->getLength() <= 0 || !dashContext) {
        utils::errorMsg("Error appeding frame to segment: frame not valid");
        return false;
    }

    timeBasePts = microsToTimeBase(vFrame->getPresentationTime());

    addSampleReturn = add_video_sample(vFrame->getDataBuf(), vFrame->getLength(), timeBasePts, 
                                        timeBasePts, segment->getSeqNumber(), isIntra, &dashContext);

    vFrame->setLength(0);

    if (addSampleReturn != I2OK) {
        utils::errorMsg("Error adding video sample. Code error: " + std::to_string(addSampleReturn));
        return false;
    }

    return true;
}

bool DashVideoSegmenter::appendNalToFrame(unsigned char* nalData, unsigned nalDataLength, 
                                           unsigned nalWidth, unsigned nalHeight, std::chrono::microseconds ts)
{
    if (vFrame->getLength() + nalDataLength + AVCC_HEADER_BYTES_MINUS_ONE + 1 > vFrame->getMaxLength()) {
        utils::errorMsg("[DashVideoSegmenter::appendNalToFrame] Nal exceeds frame max length");
        return false;
    }

    vFrame->getDataBuf()[vFrame->getLength()] = (nalDataLength >> 24) & 0xFF;
    vFrame->getDataBuf()[vFrame->getLength()+1] = (nalDataLength >> 16) & 0xFF;
    vFrame->getDataBuf()[vFrame->getLength()+2] = (nalDataLength >> 8) & 0xFF;
    vFrame->getDataBuf()[vFrame->getLength()+3] = nalDataLength & 0xFF;
    vFrame->setLength(vFrame->getLength() + 4);

    memcpy(vFrame->getDataBuf() + vFrame->getLength(), nalData, nalDataLength);
    vFrame->setLength(vFrame->getLength() + nalDataLength);

    vFrame->setSize(nalWidth, nalHeight);
    vFrame->setPresentationTime(ts);
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
