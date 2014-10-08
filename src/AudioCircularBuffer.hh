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

#define BUFFERING_SIZE_TIME 500 //ms
#define BUFFERING_THRESHOLD 100 //ms


 class AudioCircularBuffer : public FrameQueue {

    public:
        static AudioCircularBuffer* createNew(int ch, int sRate, int maxSamples, SampleFmt sFmt);
        ~AudioCircularBuffer();
        void setOutputFrameSamples(int samples); 

        Frame *getRear();
        Frame *getFront();
        void addFrame();
        void removeFrame();
        void flush();
        Frame *forceGetRear();
        Frame *forceGetFront(bool &newFrame);
        bool frameToRead() {return false;};
        int getFreeSamples();

    protected:
        bool config();


    private:
        AudioCircularBuffer(int ch, int sRate, int maxSamples, SampleFmt sFmt);

        enum State {BUFFERING, OK, FULL};

        bool pushBack(unsigned char **buffer, int samplesRequested);
        bool forcePushBack(unsigned char **buffer, int samplesRequested);
        bool popFront(unsigned char **buffer, int samplesRequested);
        void fillOutputBuffers(unsigned char **buffer, int bytesRequested);

        int channels;
        int sampleRate;
        int bytesPerSample;
        int chMaxSamples;
        int channelMaxLength;
        int delayBytes;
        unsigned char *data[MAX_CHANNELS];
        SampleFmt sampleFormat;
        bool outputFrameAlreadyRead;

        int samplesBufferingThreshold;
        State state;

        PlanarAudioFrame* inputFrame;
        PlanarAudioFrame* outputFrame;
};

#endif