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

#ifndef _AUDIO_CIRCULAR_BUFFER_HH
#define _AUDIO_CIRCULAR_BUFFER_HH

#include <atomic>
#include "Types.hh"
#include "FrameQueue.hh"
#include "AudioFrame.hh"

 class AudioCircularBuffer : public FrameQueue {

    public:
        static AudioCircularBuffer* createNew(unsigned int ch, unsigned int sRate, unsigned int maxSamples, SampleFmt sFmt);
        ~AudioCircularBuffer();
        void setOutputFrameSamples(int samples); 

        Frame *getRear();
        Frame *getFront();
        void addFrame();
        void removeFrame();
        void flush();
        Frame *forceGetRear();
        Frame *forceGetFront();
        int delay; //(ms)
        bool frameToRead() {};
        int getFreeSamples();

    protected:
        bool config();


    private:
        AudioCircularBuffer(unsigned int ch, unsigned int sRate, unsigned int maxSamples, SampleFmt sFmt);

        bool pushBack(unsigned char **buffer, int samplesRequested);
        bool popFront(unsigned char **buffer, int samplesRequested);
        bool forcePushBack(unsigned char **buffer, int samplesRequested);
        
        std::atomic<int> byteCounter;
        unsigned channels;
        unsigned sampleRate;
        unsigned bytesPerSample;
        unsigned chMaxSamples;
        unsigned channelMaxLength;
        unsigned char *data[MAX_CHANNELS];
        SampleFmt sampleFormat;
        bool outputFrameAlreadyRead;

        PlanarAudioFrame* inputFrame;
        PlanarAudioFrame* outputFrame;
};

#endif