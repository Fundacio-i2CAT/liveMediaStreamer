/*
 *  DashAudioSegmenter.cpp - DASH audio stream segmenter
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
 *
 */

 #include "DashAudioSegmenter.hh"

DashAudioSegmenter::DashAudioSegmenter(std::chrono::seconds segDur, std::chrono::microseconds offset) :
DashSegmenter(segDur, 0, offset)
{

}

DashAudioSegmenter::~DashAudioSegmenter()
{

}

Frame* DashAudioSegmenter::manageFrame(Frame* frame)
{
    AudioFrame* aFrame;

    aFrame = dynamic_cast<AudioFrame*>(frame);

    if (!aFrame) {
        utils::errorMsg("Error managing frame: it MUST be an audio frame");
        return NULL;
    }

    if (!setup(aFrame->getChannels(), aFrame->getSampleRate(), aFrame->getSamples(),
        utils::getBytesPerSampleFromFormat(aFrame->getSampleFmt())*BYTE_TO_BIT)) {
        utils::errorMsg("Error during Dash Audio Segmenter setup");
        return NULL;
    }

    if (!updateExtradata(aFrame)) {
        utils::errorMsg("[DashAudioSegmenter::manageFrame] Error updating metadata");
        return NULL;
    }

    return aFrame;
}

bool DashAudioSegmenter::setup(size_t channels, size_t sampleRate, size_t samples, size_t bitsPerSample)
{
    uint8_t i2error = I2OK;

    if (!dashContext) {
        i2error = generate_context(&dashContext, AUDIO_TYPE);
    }

    if (i2error != I2OK || !dashContext) {
        return false;
    }

    timeBase = sampleRate;
    frameDuration = samples;
    segDurInTimeBaseUnits = segDur.count()*timeBase;

    i2error = fill_audio_context(&dashContext, channels, sampleRate, bitsPerSample, timeBase, frameDuration);

    if (i2error != I2OK) {
        return false;
    }

    set_segment_duration(segDurInTimeBaseUnits, &dashContext);
    return true;
}

bool DashAudioSegmenter::generateInitSegment(DashSegment* segment)
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

    initSize = init_audio_handler(data, dataLength, segment->getDataBuffer(), &dashContext);

    if (initSize == 0) {
        return false;
    }

    segment->setDataLength(initSize);
    extradata.clear();
    return true;
}

unsigned DashAudioSegmenter::customGenerateSegment(unsigned char *segBuffer, std::chrono::microseconds nextFrameTs, 
                                                    uint64_t &segTimestamp, uint32_t &segDuration, bool force)
{
    unsigned segSize;

    if (force) {
        segSize = force_generate_audio_segment(segBuffer, &dashContext, &segTimestamp, &segDuration);
    } else {
        segSize = generate_audio_segment(segBuffer, &dashContext, &segTimestamp, &segDuration);
    }

    return segSize;
}

bool DashAudioSegmenter::appendFrameToDashSegment(Frame* frame)
{
    unsigned char* dataWithoutADTS;
    size_t dataLengthWithoutADTS;
    size_t addSampleReturn;
    uint32_t timeBasePts;

    if (!frame || !frame->getDataBuf() || frame->getLength() <= 0 || !dashContext) {
        utils::errorMsg("Error appeding frame to segment: frame not valid");
        return false;
    }
    
    timeBasePts = microsToTimeBase(frame->getPresentationTime());

    dataWithoutADTS = frame->getDataBuf() + ADTS_HEADER_LENGTH;
    dataLengthWithoutADTS = frame->getLength() - ADTS_HEADER_LENGTH;

    addSampleReturn = add_audio_sample(dataWithoutADTS, dataLengthWithoutADTS, frameDuration, 
                                        timeBasePts, timeBasePts, sequenceNumber, &dashContext);

    if (addSampleReturn != I2OK) {
        utils::errorMsg("Error adding audio sample. Code error: " + std::to_string(addSampleReturn));
        return false;
    }

    return true;
}

bool DashAudioSegmenter::flushDashContext()
{
    if (!dashContext) {
        return false;
    }

    context_refresh(&dashContext, AUDIO_TYPE);
    return true;
}

bool DashAudioSegmenter::updateExtradata(AudioFrame* aFrame)
{
    unsigned char* data;

    if (!aFrame || !aFrame->getDataBuf() || aFrame->getLength() < ADTS_HEADER_LENGTH) {
        utils::errorMsg("ADTS header not valid - check data buffer and its header length");
        return false;
    }

    data = aFrame->getDataBuf();

    if (data[0] != ADTS_FIRST_RESERVED_BYTE || data[1] != ADTS_SECOND_RESERVED_BYTE) {
        utils::errorMsg("ADTS header not valid - check reserved bytes");
        return false;
    }

    if (profile == getProfileFromADTSHeader(data) && samplingFrequencyIndex == getSamplingFreqIdxFromADTSHeader(data) &&
        channelConfiguration == getChannelConfFromADTSHeader(data) && !extradata.empty())
    {
        return true;
    }

    profile = getProfileFromADTSHeader(data);
    audioObjectType = profile + 1;
    samplingFrequencyIndex = getSamplingFreqIdxFromADTSHeader(data);
    channelConfiguration = getChannelConfFromADTSHeader(data);

    extradata.clear();
    extradata.insert(extradata.end(), getExtradata1stByte(audioObjectType, samplingFrequencyIndex));
    extradata.insert(extradata.end(), getExtradata2ndByte(samplingFrequencyIndex, channelConfiguration));

    return true;
}

unsigned char DashAudioSegmenter::getProfileFromADTSHeader(unsigned char* adtsHeader)
{
    return (adtsHeader[2] >> 6) & 0x03;
}

unsigned char DashAudioSegmenter::getSamplingFreqIdxFromADTSHeader(unsigned char* adtsHeader)
{
    return (adtsHeader[2] >> 2) & 0x0F;
}

unsigned char DashAudioSegmenter::getChannelConfFromADTSHeader(unsigned char* adtsHeader)
{
    return (adtsHeader[2] & 0x01) | ((adtsHeader[3] >> 6) & 0x03);
}

unsigned char DashAudioSegmenter::getExtradata1stByte(unsigned char audioObjectType, unsigned char samplingFrequencyIndex)
{
    return (audioObjectType<<3) | (samplingFrequencyIndex>>1);
}

unsigned char DashAudioSegmenter::getExtradata2ndByte(unsigned char samplingFrequencyIndex, unsigned char channelConfiguration)
{
    return (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
}

size_t DashAudioSegmenter::getSampleRate()
{
    if (!dashContext || !dashContext->ctxaudio) {
        return 0;
    }

    return dashContext->ctxaudio->sample_rate;
}

size_t DashAudioSegmenter::getChannels()
{
    if (!dashContext || !dashContext->ctxaudio) {
        return 0;
    }

    return dashContext->ctxaudio->channels;
}
