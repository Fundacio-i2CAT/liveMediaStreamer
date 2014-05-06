/*
 *  AudioDecoderLibab - Audio circular buffer
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _AUDIOCIRCULARBUFFER_HH
#define _AUDIOCIRCULARBUFFER_HH

#include <atomic>
#include "types.hh"

 class AudioCircularBuffer {

    public:
        AudioCircularBuffer(unsigned int chNumber, unsigned int chMaxLength, unsigned int bytesPerSmpl);
        ~AudioCircularBuffer();

        bool pushBack(unsigned char **buffer, int samplesRequested);
        bool popFront(unsigned char **buffer, int samplesRequested);

    private:
        std::atomic<int> byteCounter;
        std::atomic<int> front;
        std::atomic<int> rear;
        unsigned int channels;
        unsigned int bytesPerSample;
        unsigned int chMaxSamples;
        unsigned int channelMaxLength;
        unsigned char *data[MAX_CHANNELS];
};

#endif