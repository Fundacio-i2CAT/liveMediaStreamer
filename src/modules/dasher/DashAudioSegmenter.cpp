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

DashAudioSegmenter::DashAudioSegmenter(size_t segDur, std::string segBaseName) : 
DashSegmenter(segDur, 0, segBaseName)
{

}

DashAudioSegmenter::~DashAudioSegmenter()
{

}

bool DashAudioSegmenter::manageFrame(Frame* frame)
{
    AudioFrame* aFrame;

    aFrame = dynamic_cast<AudioFrame*>(frame);

    if (!aFrame) {
        utils::errorMsg("Error managing frame: it MUST be an audio frame");
        return false;
    }

    if (!updateTimeValues(aFrame->getPresentationTime().count(), aFrame->getSampleRate(), aFrame->getSamples())) {
        utils::errorMsg("Error setting time values in DashAudioSegmenter: sample rate or number of samples values not valid");
        return false;
    }

    if (!setup(customSegmentDuration, timeBase, frameDuration, aFrame->getChannels(), 
                        aFrame->getSampleRate(), aFrame->getBytesPerSample()*BYTE_TO_BIT)) {
        utils::errorMsg("Error during Dash Audio Segmenter setup");
        return false;
    }

    if (parseADTSHeader(aFrame) && generateInit()) {
        if(!initSegment->writeToDisk(getInitSegmentName())) {
            utils::errorMsg("Error writing DASH init segment to disk: invalid path");
            return false;
        }
    }

    if (appendFrameToDashSegment(aFrame->getDataBuf(), aFrame->getLength(), customTimestamp(aFrame->getPresentationTime().count()))) {
        if(!segment->writeToDisk(getSegmentName())) {
            utils::errorMsg("Error writing DASH segment to disk: invalid path");
            return false;
        }

        segment->incrSeqNumber();
    }
    return true;
}

bool DashAudioSegmenter::finishSegment()
{

}


bool DashAudioSegmenter::appendFrameToDashSegment(unsigned char* data, unsigned dataLength, size_t pts)
{
    size_t segmentSize = 0;
    unsigned char* dataWithoutADTS;
    size_t dataLengthWithoutADTS;

    if (!data || dataLength <= 0) {
        return false;
    }

    dataWithoutADTS = data + ADTS_HEADER_LENGTH;
    dataLengthWithoutADTS = dataLength - ADTS_HEADER_LENGTH;

    segment->setTimestamp(dashContext->ctxaudio->earliest_presentation_time);
    segmentSize = add_sample(dataWithoutADTS, dataLengthWithoutADTS, frameDuration, pts, pts, segment->getSeqNumber(), 
                             AUDIO_TYPE, segment->getDataBuffer(), 1, &dashContext);

    if (segmentSize <= I2ERROR_MAX) {
        return false;
    }
    
    segment->setDataLength(segmentSize);
    return true;
}

std::string DashAudioSegmenter::getSegmentName()
{
    std::string fullName;

    fullName = baseName + "_" + std::to_string(segment->getTimestamp()) + ".m4a";
    
    return fullName;
}

std::string DashAudioSegmenter::getInitSegmentName()
{
    std::string fullName;

    fullName = baseName + "_init.m4a";
    
    return fullName;
}

bool DashAudioSegmenter::generateInit() 
{
    size_t initSize = 0;
    unsigned char* data;
    unsigned dataLength;

    if (!dashContext || metadata.empty()) {
        return false;
    }

    data = reinterpret_cast<unsigned char*> (&metadata[0]);
    dataLength = metadata.size();

    if (!data) {
        return false;
    }

    initSize = init_audio_handler(data, dataLength, initSegment->getDataBuffer(), &dashContext);

    if (initSize == 0) {
        return false;
    }

    initSegment->setDataLength(initSize);

    return true;
}

bool DashAudioSegmenter::updateTimeValues(size_t currentTimestamp, int sampleRate, int samples) 
{
    if (sampleRate <= 0 || samples <= 0) {
        return false;
    }

    if (tsOffset <= 0) {
        tsOffset = currentTimestamp;
    }

    frameDuration = samples;
    timeBase = sampleRate;
    customSegmentDuration = (segmentDuration*timeBase)/MICROSECONDS_TIME_BASE;
    return true;
}

size_t DashAudioSegmenter::customTimestamp(size_t currentTimestamp) 
{
    return ((currentTimestamp - tsOffset)*timeBase)/MICROSECONDS_TIME_BASE;
}

bool DashAudioSegmenter::parseADTSHeader(AudioFrame* frame)
{
    unsigned char* data;

    if (!frame->getDataBuf() || frame->getLength() < ADTS_HEADER_LENGTH) {
        utils::errorMsg("ADTS header not valid");
        return false;
    }

    data = frame->getDataBuf();

    if (data[0] != ADTS_FIRST_RESERVED_BYTE || data[1] != ADTS_SECOND_RESERVED_BYTE) {
        utils::errorMsg("ADTS header not valid");
        return false;
    }

    if (profile == getProfileFromADTSHeader(data) && samplingFrequencyIndex == getSamplingFreqIdxFromADTSHeader(data) &&
        channelConfiguration == getChannelConfFromADTSHeader(data) && !metadata.empty()) 
    {
        return false;
    }

    profile = getProfileFromADTSHeader(data);
    audioObjectType = profile + 1;
    samplingFrequencyIndex = getSamplingFreqIdxFromADTSHeader(data);
    channelConfiguration = getChannelConfFromADTSHeader(data);

    metadata.clear();
    metadata.insert(metadata.end(), getMetadata1stByte(audioObjectType, samplingFrequencyIndex));
    metadata.insert(metadata.end(), getMetadata2ndByte(samplingFrequencyIndex, channelConfiguration));

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

unsigned char DashAudioSegmenter::getMetadata1stByte(unsigned char audioObjectType, unsigned char samplingFrequencyIndex)
{
    return (audioObjectType<<3) | (samplingFrequencyIndex>>1);
}

unsigned char DashAudioSegmenter::getMetadata2ndByte(unsigned char samplingFrequencyIndex, unsigned char channelConfiguration)
{
    return (samplingFrequencyIndex<<7) | (channelConfiguration<<3);
}

bool DashAudioSegmenter::setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t channels, size_t sampleRate, size_t bitsPerSample)
{
    uint8_t i2error = I2OK;

    if (!dashContext) {
        i2error = generate_context(&dashContext, AUDIO_TYPE);
    }
    
    if (i2error != I2OK || !dashContext) {
        return false;
    }

    i2error = fill_audio_context(&dashContext, channels, sampleRate, bitsPerSample, timeBase, sampleDuration); 

    if (i2error != I2OK) {
        return false;
    }

    set_segment_duration(segmentDuration, &dashContext);
    return true;
}