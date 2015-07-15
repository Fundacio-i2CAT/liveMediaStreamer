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

/*! Filter that mixes different audio frames in one frame. Each mixing channel is 
*   identified by and Id which coincides with the reader associated to it. 
*/

class AudioMixer : public ManyToOneFilter {

public:
    /**
    * Class constructor
    * @param inputChannels Max mixing channels
    */
    AudioMixer(int inputChannels = AMIXER_MAX_CHANNELS);

    /**
    * Class destructor
    */
    ~AudioMixer();

    /**
    * It converts a sample from its bytes representation to its float representation
    * @param origin (in) Pointer to sample bytes
    * @param dst (out) Float value
    * @param fmt (in) Sample format (only S16P and FLTP are supported)
    * @return true on success and false if not
    */
    static bool bytesToFloat(unsigned char const* origin, float &dst, SampleFmt fmt);

    /**
    * It converts a sample from its float reapresentation to its bytes representation
    * @param dst (out) Pointer to the sample buffer
    * @param origin (in) Float sample value
    * @param fmt (in) Sample format (only S16P and FLTP are supported)
    * @return true on success and false if not
    */ 
    static bool floatToBytes(unsigned char* dst, float const origin, SampleFmt fmt);

    /**
    * @return mixing buffering in samples
    */ 
    unsigned getMixingThreshold() {return mixingThreshold;};

    /**
    * @return samples requested to each mixing channel
    */ 
    unsigned getInputFrameSamples() {return inputFrameSamples;};

    /**
    * Sets channel gain
    * @param id channel id
    * @param value channel gain [0.0, 1.0]
    * @return always true
    */ 
    bool changeChannelGain(int id, float value);

    /**
    * Mute channel
    * @param id channel id
    * @return always true
    */ 
    bool muteChannel(int id);

    /**
    * Mute all channels except one
    * @param id channel id 
    * @return always true
    */ 
    bool soloChannel(int id);

    /**
    * Sets master gain
    * @param value master gain [0.0, 1.0]
    * @return always true
    */ 
    bool changeMasterGain(float value);

    /**
    * Mutes master
    * @return always true
    */ 
    bool muteMaster();

protected:
    std::shared_ptr<Reader> setReader(int readerID, FrameQueue* queue);
    void doGetState(Jzon::Object &filterNode);
    FrameQueue *allocQueue(struct ConnectionData cData);
    bool doProcessFrame(std::map<int, Frame*> &orgFrames, Frame *dst);

private:
    void initializeEventMap();
    bool pushToBuffer(int mixChId, AudioFrame* frame);
    bool fillChannel(std::queue<float> &buffer, int nOfSamples, unsigned char* data, SampleFmt fmt); 
    bool extractMixedFrame(AudioFrame* frame);
    void mixSample(float sample, float* mixBuff, int bufferIdx, float gain);
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
    float th;  //Dynamic Range Compression algorithm threshold

    std::map<int, float> gains;
    std::chrono::microseconds syncTs;
    float* mixBuffers[MAX_CHANNELS];

    unsigned mixBufferMaxSamples;
    unsigned outputSamples;
    unsigned mixingThreshold;


};


#endif
