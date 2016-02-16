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

#define ADTS_FIRST_RESERVED_BYTE 0XFF
#define ADTS_SECOND_RESERVED_BYTE 0XF1
#define ADTS_HEADER_LENGTH 7

#include "Dasher.hh"

/*! Class responsible for managing DASH audio segments creation. It receives AAC frames appending them to create
    complete segments. It also manages Init Segment creation, constructing MP4 metadata from AAC frames ADTS header*/

class DashAudioSegmenter : public DashSegmenter {

public:
    /**
    * Class constructor
    * @param segDur Segment duration in milliseconds
    * @param offset of the initial timestamp
    */
    DashAudioSegmenter(std::chrono::seconds segDur, std::chrono::microseconds offset);

    /**
    * Class destructor
    */
    ~DashAudioSegmenter();

    /**
    * It checks that the received frame is an audio frame and stores the pointer of this frame internally
    * @param frame Pointer the source frame, which must be contained in an AudioFrame structure
    * @param newFrame Passed by referenc.In audio case, it is set always to true
    * @return false if error and true if the NAL has been managed correctly
    */
    Frame* manageFrame(Frame* frame);

    /**
    * It returns the last configured sample rate
    * @return sample rate in Hz
    */
    size_t getSampleRate();
    
    /**
    * It returns the last configured audio channels number
    * @return number of audio channels
    */
    size_t getChannels();

    bool appendFrameToDashSegment(Frame* frame);
    bool flushDashContext();
    bool generateInitSegment(DashSegment* segment);

private:
    bool updateExtradata(AudioFrame* aFrame);
    unsigned customGenerateSegment(unsigned char *segBuffer, std::chrono::microseconds nextFrameTs, 
                                    uint64_t &segTimestamp, uint32_t &segDuration, bool force);


    bool setup(size_t channels, size_t sampleRate, size_t samples, size_t bitsPerSample);
    unsigned char getProfileFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getSamplingFreqIdxFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getChannelConfFromADTSHeader(unsigned char* adtsHeader);
    unsigned char getExtradata1stByte(unsigned char audioObjectType, unsigned char samplingFrequencyIndex);
    unsigned char getExtradata2ndByte(unsigned char samplingFrequencyIndex, unsigned char channelConfiguration);

    unsigned char profile;
    unsigned char audioObjectType;
    unsigned char samplingFrequencyIndex;
    unsigned char channelConfiguration;
};

#endif
