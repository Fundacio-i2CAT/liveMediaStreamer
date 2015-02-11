/*
 *  DashAudioSegmenter.hh - DASH audio stream segmenter
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
 
#ifndef _DASH_AUDIO_SEGMENTER_HH
#define _DASH_AUDIO_SEGMENTER_HH

#include "Dasher.hh"

class DashAudioSegmenter : public DashSegmenter {

public:
    DashAudioSegmenter(size_t segDur, std::string segBasename);
    ~DashAudioSegmenter();
    bool manageFrame(Frame* frame);
    bool updateConfig();
    bool finishSegment();

private:
    bool updateMetadata();
    bool generateInitData();
    bool appendFrameToDashSegment();

    bool setup(size_t segmentDuration, size_t timeBase, size_t sampleDuration, size_t channels, size_t sampleRate, size_t bitsPerSample);
    unsigned char getProfileFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getSamplingFreqIdxFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getChannelConfFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getMetadata1stByte(unsigned char audioObjectType, unsigned char samplingFrequencyIndex);
    unsigned char getMetadata2ndByte(unsigned char samplingFrequencyIndex, unsigned char channelConfiguration);
    bool updateTimeValues(size_t currentTimestamp, int sampleRate, int samples);
    size_t customTimestamp(size_t currentTimestamp);

    unsigned char profile;
    unsigned char audioObjectType;
    unsigned char samplingFrequencyIndex;
    unsigned char channelConfiguration;
    size_t tsOffset;
    size_t customSegmentDuration;
    AudioFrame* aFrame;
};

#endif
