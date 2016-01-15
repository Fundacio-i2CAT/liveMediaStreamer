/*
 *  DecoderPi.hh - Specific Class to decode video over RaspberryPi.
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
 *  Authors:  Alejandro Jiménez <alejandro.jimenez@i2cat.net>
 *
 */

#ifndef _DECODER_PI_HH
#define _DECODER_PI_HH


#include "../../Filter.hh"
#include "../../VideoFrame.hh"
#include "../../AudioFrame.hh"
#include "../../StreamInfo.hh"

#include <map>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <bcm_host.h>
#include <ilclient.h>



struct OPENMAX_H264_DECODER {
    ILCLIENT_T *client;
    COMPONENT_T *decode;
    COMPONENT_T *scheduler;
    COMPONENT_T *clock;
    COMPONENT_T *render;
    COMPONENT_T *list[5];
    TUNNEL_T tunnel[4];
    OMX_VIDEO_PARAM_PORTFORMATTYPE format;
    OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
    int first_packet;
    int port_settings_changed;
};


class DecoderPi : public TailFilter {
public:
    static DecoderPi* createNew(unsigned readersNum = 1, std::chrono::microseconds fTime = std::chrono::microseconds(0));

    ~DecoderPi();

    bool configure(int fTime);
    int getConfigure() {return getFrameTime().count();};

protected:
    DecoderPi(unsigned readersNum, std::chrono::microseconds fTime);
    bool doProcessFrame(std::map<int, Frame*> &orgFrames, std::vector<int> newFrames);
    void doGetState(Jzon::Object &filterNode);
    bool configure0(std::chrono::microseconds fTime);

private:
    void initializeEventMap();

    bool specificReaderConfig(int /*readerID*/, FrameQueue* /*queue*/)  {return true;};
    bool specificReaderDelete(int /*readerID*/) {return true;};

    bool configureEvent(Jzon::Node* params);


    bool omxInit(OPENMAX_H264_DECODER *d);
    bool omxDecode(OPENMAX_H264_DECODER *d,  VideoFrame* vFrame);
    bool omxCleanup(OPENMAX_H264_DECODER *d);


    std::chrono::microseconds timestampOffset;
    OPENMAX_H264_DECODER piH264Decoder;
};

#endif
