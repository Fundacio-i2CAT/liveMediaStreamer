/*
 *  AudioFrameQueue - A lock-free audio frame circular queue
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
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

#ifndef _AUDIO_FRAME_QUEUE_HH
#define _AUDIO_FRAME_QUEUE_HH

#ifndef _FRAME_QUEUE_HH
#include "FrameQueue.hh"
#endif

#include <atomic>
#include <sys/time.h>
#include <chrono>
#include "Types.hh"

using namespace std::chrono;

class AudioFrameQueue : public FrameQueue {

public:
    static AudioFrameQueue* createNew(ACodecType codec,  unsigned delay, unsigned sampleRate = 48000, unsigned channels = 2, SampleFmt sFmt = S16);

protected:
    AudioFrameQueue(ACodecType codec, SampleFmt sFmt, unsigned sampleRate, unsigned channels, unsigned delay);
    ACodecType codec;
    SampleFmt sampleFormat;
    unsigned sampleRate;
    unsigned channels;
    bool config();

};

#endif