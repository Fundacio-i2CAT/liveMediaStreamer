/*
 *  AudioMixer - Audio mixer structure
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

#ifndef _AUDIO_MIXER_HH
#define _AUDIO_MIXER_HH

#include "../../Frame.hh"
#include "../../Filter.hh"
#include "../../AudioFrame.hh"

#define COMPRESSION_THRESHOLD 0.6
#define DEFAULT_MASTER_GAIN 0.6
#define DEFAULT_CHANNEL_GAIN 1.0
#define AMIXER_MAX_CHANNELS 16

enum mixingAlgorithm
{
    LA,      //Linear Attenuation
    LDRC     //Linear Dynamic Range Compression
};

class AudioMixer : public ManyToOneFilter {

public:
    AudioMixer(FilterRole fRole_ = MASTER, bool sharedFrames = true, int inputChannels = AMIXER_MAX_CHANNELS);
    ~AudioMixer();
    FrameQueue *allocQueue(int wId);

private:
    Reader *setReader(int readerID, FrameQueue* queue);
    void doGetState(Jzon::Object &filterNode);
    bool doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst);
    void initializeEventMap();
    bool pushToBuffer(int id, AudioFrame* frame);
    bool fillChannel(std::queue<float> &buffer, int nOfSamples, unsigned char* data, SampleFmt fmt); 
    bool bytesToFloat(unsigned char const* origin, float &dst, SampleFmt fmt); 
    bool floatToBytes(unsigned char* dst, float const origin, SampleFmt fmt);
    bool extractMixedFrame(AudioFrame* frame);
    void mixSample(float sample, int bufferPosition);
    bool setChannelGain(int id, float value);

    bool changeChannelVolumeEvent(Jzon::Node* params);
    bool muteChannelEvent(Jzon::Node* params);
    bool soloChannelEvent(Jzon::Node* params);
    bool changeMasterVolumeEvent(Jzon::Node* params);
    bool muteMasterEvent(Jzon::Node* params);

    int channels;
    int sampleRate;
    int inputFrameSamples;
    SampleFmt sampleFormat;
    int maxMixingChannels;
    unsigned front;
    unsigned rear;

    float masterGain;
    float th;  //Dynamic Range Compression algorithms threshold
    mixingAlgorithm mAlg;

    std::map<int, float> gains;
    std::chrono::microseconds syncTs;
    float* mixBuffers[MAX_CHANNELS];

    unsigned mixBufferMaxSamples;
    unsigned outputSamples;
    unsigned mixingThreshold;


};


#endif
