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

#define COMPRESSION_THRESHOLD 0.6
#define DEFAULT_MASTER_GAIN 0.6
#define DEFAULT_CHANNEL_GAIN 1.0
#define AMIXER_MAX_CHANNELS 8


enum mixingAlgorithm
{
    LA,      //Linear Attenuation
    LDRC     //Linear Dynamic Range Compression
}; 

class AudioMixer : public ManyToOneFilter {
    
    public:
        AudioMixer(int inputChannels = AMIXER_MAX_CHANNELS);
        AudioMixer(int inputChannels, int frameChannels, int sampleRate);
        FrameQueue *allocQueue(int wId);
        bool doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst);

    protected:
        Reader *setReader(int readerID, FrameQueue* queue);
        void initializeEventMap();
        void doGetState(Jzon::Object &filterNode);

    private:
        void mixNonEmptyFrames(std::map<int, Frame*> orgFrames, std::vector<int> filledFramesIds, Frame *dst); 
        void applyMixAlgorithm(std::vector<float> &fSamples, int frameNumber, int realFrameNumber);
        void applyGainToChannel(std::vector<float> &fSamples, float gain);
        void sumValues(std::vector<float> fSamples, std::vector<float> &mixedSamples); 
        void LAMixAlgorithm(std::vector<float> &fSamples, int frameNumber); 
        void LDRCMixAlgorithm(std::vector<float> &fSamples, int frameNumber); 
        
        void changeChannelVolumeEvent(Jzon::Node* params, Jzon::Object &outputNode); 
        void muteChannelEvent(Jzon::Node* params, Jzon::Object &outputNode); 
        void soloChannelEvent(Jzon::Node* params, Jzon::Object &outputNode);
        void changeMasterVolumeEvent(Jzon::Node* params, Jzon::Object &outputNode);
        void muteMasterEvent(Jzon::Node* params, Jzon::Object &outputNode);

        int frameChannels;
        int sampleRate;
        SampleFmt sampleFormat;
        std::map<int,float> gains;
        float masterGain;
        float th;  //Dynamic Range Compression algorithms threshold
        mixingAlgorithm mAlg;

        //Vectors as attributes in order to improve memory management
        std::vector<float> samples;
        std::vector<float> mixedSamples;
};


#endif