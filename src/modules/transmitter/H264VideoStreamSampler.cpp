/*
 *  CustomH264or5VideoDiscreteFramer.cpp - Custom H264or5VideoDiscreteFramer liveMedia class.
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
 */

#include "H264VideoStreamSampler.hh"
extern "C" { 
    #include "SPSparser/h264_stream.h"
}

H264VideoStreamSampler* H264VideoStreamSampler::createNew(UsageEnvironment& env, FramedSource* inputSource, bool annexB)
{
    return new H264VideoStreamSampler(env, inputSource, annexB);
}

 H264VideoStreamSampler
::H264VideoStreamSampler(UsageEnvironment& env, FramedSource* inputSource, bool annexB)
  : H264or5VideoStreamFramer(264, env, inputSource, False/*don't create a parser*/, False),
  offset(0), totalFrameSize(0), fAnnexB(annexB), fWidth(0), fHeight(0) {
}

H264VideoStreamSampler::~H264VideoStreamSampler() {
}

void H264VideoStreamSampler::doGetNextFrame() {
    fInputSource->getNextFrame(fTo + offset + NAL_START_SIZE, fMaxSize - offset - NAL_START_SIZE,
                               afterGettingFrame, this, FramedSource::handleClosure, this);
}

void H264VideoStreamSampler
::afterGettingFrame(void* clientData, unsigned frameSize,
                    unsigned numTruncatedBytes,
                    struct timeval presentationTime,
                    unsigned durationInMicroseconds) {
  H264VideoStreamSampler* source = (H264VideoStreamSampler*)clientData;
  source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void H264VideoStreamSampler
::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
                     struct timeval presentationTime,
                     unsigned durationInMicroseconds) 
{
    unsigned char* realPtr = fTo + offset;
    unsigned char* NALstartPtr = realPtr + NAL_START_SIZE;

    if (fAnnexB) {
        realPtr[0] = 0;
        realPtr[1] = 0;
        realPtr[2] = 0;
        realPtr[3] = 1;
    } else {
        uint32_t nalSizeHtoN = htonl(frameSize);
        memcpy(realPtr, &nalSizeHtoN, NAL_START_SIZE);
    }


    u_int8_t nal_unit_type;
    if (frameSize >= 1) {
        nal_unit_type = NALstartPtr[0]&0x1F;
    } else {
        // This is too short to be a valid NAL unit, so just assume a bogus nal_unit_type
        nal_unit_type = 0xFF;
    }



    if (frameSize >= 4 && NALstartPtr[0] == 0 && NALstartPtr[1] == 0 && ((NALstartPtr[2] == 0 && NALstartPtr[3] == 1) || NALstartPtr[2] == 1)) {
        envir() << "H264VideoStreamSampler error: MPEG 'start code' seen in the input\n";
    } else if (isSPS(nal_unit_type)) { // Sequence parameter set (SPS)
        saveCopyOfSPS(NALstartPtr, frameSize);
        updateSPSInfo(NALstartPtr, frameSize);
    } else if (isPPS(nal_unit_type)) { // Picture parameter set (PPS)
        saveCopyOfPPS(NALstartPtr, frameSize);
    }

    offset += frameSize + NAL_START_SIZE;
    totalFrameSize = offset;

    if (isAUD(nal_unit_type) || numTruncatedBytes > 0) {
        fPictureEndMarker = True;
        fFrameSize = offset - frameSize - NAL_START_SIZE;
        fNumTruncatedBytes = numTruncatedBytes;
        fPresentationTime = presentationTime;
        fDurationInMicroseconds = durationInMicroseconds;

        if (numTruncatedBytes > 0) {
            envir() << "H264VideoStreamSampler error: Frame buffer too small\n";
        }

        resetInternalValues();
        afterGetting(this);
    } else {
        doGetNextFrame();
    }
}

void H264VideoStreamSampler::resetInternalValues()
{
    offset = 0;
    totalFrameSize = 0;
}

void H264VideoStreamSampler::updateSPSInfo(unsigned char* NALstartPtr, int frameSize)
{
    uint32_t width = 0;
    uint32_t height = 0;
    int timeScale = 0;
    int num_units_in_tick = 0;

    sps_t* sps = (sps_t*)malloc(sizeof(sps_t));
    uint8_t* rbsp_buf = (uint8_t*)malloc(frameSize);
    if (nal_to_rbsp(NALstartPtr, &frameSize, rbsp_buf, &frameSize) < 0){
        free(rbsp_buf);
        free(sps);
    	envir() << "H264VideoStreamSampler::updateVideoSize error: nal to rbsp failed!\n";
        return;
    }

    bs_t* b = bs_new(rbsp_buf, frameSize);
    if(read_seq_parameter_set_rbsp(sps,b) < 0){
        bs_free(b);
        free(rbsp_buf);
        free(sps);
    	envir() << "H264VideoStreamSampler::updateVideoSize error: read sequence parameter and set rbsp failed!\n";
        return;
    }

    timeScale = sps->vui.time_scale;
    num_units_in_tick = sps->vui.num_units_in_tick;

    width = (sps->pic_width_in_mbs_minus1 + 1) * 16;
    height = (2 - sps->frame_mbs_only_flag) * (sps->pic_height_in_map_units_minus1 + 1) * 16;
    //NOTE: frame_mbs_only_flag = 1 --> only progressive frames
    //      frame_mbs_only_flag = 0 --> some type of interlacing (there are 3 types contemplated in the standard)
    if (sps->frame_cropping_flag){
        width -= (sps->frame_crop_left_offset*2 + sps->frame_crop_right_offset*2);
        height -= (sps->frame_crop_top_offset*2 + sps->frame_crop_bottom_offset*2);
    }

    if(width > 0){
        fWidth = width;
    }

    if(height > 0){
    	fHeight = height;
    }

    if (num_units_in_tick > 0) {
        fFrameRate = timeScale/(2*num_units_in_tick);
    }

    bs_free(b);
    free(rbsp_buf);
    free(sps);
}

bool H264VideoStreamSampler::isAUD(u_int8_t nal_unit_type)
{
    return nal_unit_type == 9;
}

int H264VideoStreamSampler::getWidth()
{
	return fWidth;
}

int H264VideoStreamSampler::getHeight()
{
    return fHeight;
}

double H264VideoStreamSampler::getFrameRate()
{
    return fFrameRate;
}
