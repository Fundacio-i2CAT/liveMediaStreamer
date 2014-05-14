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

#include "AudioFrameQueue.hh"
#include "InterleavedAudioFrame.hh"
#include "PlanarAudioFrame.hh"

unsigned getMaxSamples(unsigned sampleRate);

AudioFrameQueue* AudioFrameQueue::createNew(ACodecType codec,  unsigned delay, unsigned sampleRate, unsigned channels, SampleFmt sFmt)
{
    return new AudioFrameQueue(codec, sFmt, sampleRate, channels, delay);
}

AudioFrameQueue::AudioFrameQueue(ACodecType codec, SampleFmt sFmt, unsigned sampleRate, unsigned channels, unsigned delay)
{
    this->codec = codec;
    this->sampleFormat = sFmt;
    this->delay = delay;
    this->channels = channels;
    this->sampleRate = sampleRate;

    config();
}

bool AudioFrameQueue::config()
{
    switch(codec) {
        case OPUS:
            max = FRAMES_OPUS;
            sampleFormat = S16;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedAudioFrame(channels, sampleRate, getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        case PCMU:
        case PCM:
            max = FRAMES_AUDIO_RAW;
            if (sampleFormat == U8 || sampleFormat == S16 || sampleFormat == FLT) {
                for (int i=0; i<max; i++) {
                    frames[i] = new InterleavedAudioFrame(channels, sampleRate, getMaxSamples(sampleRate), codec, sampleFormat);
                }
            } else if (sampleFormat == U8P || sampleFormat == S16P || sampleFormat == FLTP) {
                for (int i=0; i<max; i++) {
                    frames[i] = new PlanarAudioFrame(channels, sampleRate, getMaxSamples(sampleRate), codec, sampleFormat);
                }
            } else {
                //TODO: error
            }
        case G711:
            channels = 1;
            sampleRate = 8000;
            sampleFormat = U8;
            max = FRAMES_AUDIO_RAW;
            for (int i=0; i<max; i++) {
                frames[i] = new InterleavedAudioFrame(channels, sampleRate, getMaxSamples(sampleRate), codec, sampleFormat);
            }
            break;
        default:
            //TODO: codec not supported
            break;
    }

}

unsigned getMaxSamples(unsigned sampleRate)
{
    return (AUDIO_FRAME_TIME*sampleRate)/1000;
}


