/*
 *  DecoderPi.cpp - Specific Class to decode video over RaspberryPi.
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
 */

#include "DecoderPi.hh"
#include <chrono>

DecoderPi* DecoderPi::createNew(unsigned readersNum, std::chrono::microseconds fTime)
{
    if (readersNum != 1) {
	utils::warningMsg("[DecoderPi] ReadersNum < 1");
	return NULL;
    }
    if (fTime.count() < 0) {
        utils::errorMsg("[DecoderPi] Error creating DecoderPi, negative frame time is not valid");
        return NULL;
    }

    return new DecoderPi(readersNum, fTime);
}

DecoderPi::DecoderPi(unsigned readersNum, std::chrono::microseconds fTime)
:TailFilter(readersNum)
{
    initializeEventMap();

    fType = DECODER_PI;
    setFrameTime(fTime);
    bcm_host_init();
    omxInit(&piH264Decoder);
}

DecoderPi::~DecoderPi()
{
    omxCleanup(&piH264Decoder);
}

bool DecoderPi::configure(int fTime)
{
    Jzon::Object root, params;
    root.Add("action", "configure");
    params.Add("fTime", fTime);
    root.Add("params", params);
    Event e(root, std::chrono::system_clock::now(), 0);
    pushEvent(e);
    return true;
}

bool DecoderPi::configure0(std::chrono::microseconds fTime)
{
    if (fTime.count() < 0) {
        utils::errorMsg("[VideoSplitter::configCrop0] Error, negative frame time is not valid");
        return NULL;
    }

    setFrameTime(fTime);

    return true;
}

void DecoderPi::initializeEventMap()
{
	eventMap["configure"] = std::bind(&DecoderPi::configureEvent, this, std::placeholders::_1);
}

bool DecoderPi::doProcessFrame(std::map<int, Frame*> &orgFrames, std::vector<int> newFrames)
{
    VideoFrame *vFrame;
    vFrame = dynamic_cast<VideoFrame*>(orgFrames[0]);

    if(!vFrame){
        utils::errorMsg("[DecoderPi::doProcessFrame] No origin frame");
        return false;
    }

    omxDecode(&piH264Decoder, vFrame);
    return true;
}

bool DecoderPi::configureEvent(Jzon::Node* params)
{
    if (!params)
    {
        utils::errorMsg("[DecoderPi::configureEvent] Params node missing");
        return false;
    }

    std::chrono::microseconds fTime = std::chrono::microseconds(0);

    if (params->Has("fTime") && params->Get("fTime").IsNumber())
    {
        fTime = std::chrono::microseconds(params->Get("fTime").ToInt());
    }

    return configure0(fTime);
}

void DecoderPi::doGetState(Jzon::Object &filterNode)
{
    filterNode.Add("frameTime", getConfigure());
}

bool DecoderPi::omxInit(OPENMAX_H264_DECODER *d)
{
    memset(d->tunnel, 0, sizeof(d->tunnel));
    memset(d->list, 0, sizeof(d->list));

    if ((d->client = ilclient_init()) == NULL)
    {
        utils::errorMsg("[DecoderPi::omxInit] Error starting IL Client");
        return false;
    }

    if (OMX_Init() != OMX_ErrorNone) {
        ilclient_destroy(d->client);
        utils::errorMsg("[DecoderPi::omxInit] Error starting OMX");
        return false;
    }

    if ( ilclient_create_component(d->client, &d->decode, "video_decode", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
        utils::errorMsg("[DecoderPi::omxInit] Error creating OMX_decode");
        return false;
    }

    d->list[0] = d->decode;

    if (ilclient_create_component(d->client, &d->render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
        utils::errorMsg("[DecoderPi::omxInit] Error creating OMX_render");
        return false;
    }
    d->list[1] = d->render;

    if (ilclient_create_component(d->client, &d->clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
        utils::errorMsg("[DecoderPi::omxInit] Error creating OMX_clock");
        return false;
    }
    d->list[2] = d->clock;

    memset(&d->cstate, 0, sizeof(d->cstate));
    d->cstate.nSize = sizeof(d->cstate);
    d->cstate.nVersion.nVersion = OMX_VERSION;
    d->cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
    d->cstate.nWaitMask = 1;
    if (OMX_SetParameter(ILC_GET_HANDLE(d->clock), OMX_IndexConfigTimeClockState, &d->cstate) != OMX_ErrorNone) {
        utils::errorMsg("[DecoderPi::omxInit] Error setting OMX_clock");
        return false;
    }

    if (ilclient_create_component(d->client, &d->scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0) {
        utils::errorMsg("[DecoderPi::omxInit] Error starting OMX_scheduler");
        return false;
    }

    set_tunnel(d->tunnel, d->decode, 131, d->scheduler, 10);
    set_tunnel(d->tunnel+1, d->scheduler, 11, d->render, 90);
    set_tunnel(d->tunnel+2, d->clock, 80, d->scheduler, 12);

    if (ilclient_setup_tunnel(d->tunnel+2, 0, 0) != 0) {
        utils::errorMsg("[DecoderPi::omxInit] Error setting OMX_tunnel");
        return false;
    }

    ilclient_change_component_state(d->decode, OMX_StateIdle);

    memset(&d->format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
    d->format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
    d->format.nVersion.nVersion = OMX_VERSION;
    d->format.nPortIndex = 130;
    d->format.eCompressionFormat = OMX_VIDEO_CodingAVC;

    if (OMX_SetParameter(ILC_GET_HANDLE(d->decode), OMX_IndexParamVideoPortFormat, &d->format) != OMX_ErrorNone) {
        utils::errorMsg("[DecoderPi::omxInit] Error setting OMX_decode");
        return false;
    }

    if (ilclient_enable_port_buffers(d->decode, 130, NULL, NULL, NULL) != 0) {
        utils::errorMsg("[DecoderPi::omxInit] Error starting OMX_buffers");
        return false;
    }

    ilclient_change_component_state(d->decode, OMX_StateExecuting);

    d->first_packet = 0;
    d->port_settings_changed = 0;

    return true;

}

bool DecoderPi::omxDecode(OPENMAX_H264_DECODER *d, VideoFrame* vFrame)
{
    OMX_BUFFERHEADERTYPE *buf;

    buf = ilclient_get_input_buffer(d->decode, 130, 1);
    if (buf == NULL) {
        utils::errorMsg("[DecoderPi::omxDecode] Error starting OMX_buffer");
        return false;
    }

    if (vFrame->getLength() == 0) {
        buf->nFilledLen = 0;
        buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

        if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(d->decode), buf) != OMX_ErrorNone){
            utils::errorMsg("[DecoderPi::omxDecode] Error OMX_buffer is empty");
            return false;
        }

        ilclient_wait_for_event(d->render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0, ILCLIENT_BUFFER_FLAG_EOS, 10000);

        ilclient_flush_tunnels(d->tunnel, 0);

        ilclient_disable_port_buffers(d->decode, 130, NULL, NULL, NULL);

        return true;
    }

    unsigned char *dest = buf->pBuffer;

    memcpy(&dest, vFrame->getDataBuf(), vFrame->getLength());
    buf->nFilledLen = vFrame->getLength();
    buf->nOffset = 0;

    if (d->port_settings_changed == 0 &&
    (vFrame->getLength() > 0 && ilclient_remove_event(d->decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0)) {
        d->port_settings_changed = 1;

        if (ilclient_setup_tunnel(d->tunnel, 0, 0) != 0){
            utils::errorMsg("[DecoderPi::omxDecode] Error setting OMX_tunnel");
            return false;
        }
        ilclient_change_component_state(d->scheduler, OMX_StateExecuting);

        if (ilclient_setup_tunnel(d->tunnel+1, 0, 1000) != 0){
            utils::errorMsg("[DecoderPi::omxDecode] Error setting OMX_tunnel");
            return false;
        }
        ilclient_change_component_state(d->render, OMX_StateExecuting);
    }

    if (d->first_packet) {
        buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
        d->first_packet = 0;
    } else {
        buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;
    }


    if (OMX_EmptyThisBuffer(ILC_GET_HANDLE(d->decode), buf) != OMX_ErrorNone){
        utils::errorMsg("[DecoderPi::omxDecode] Error OMX_buffer is empty");
        return false;
    }
    
    return true;
}

bool DecoderPi::omxCleanup(OPENMAX_H264_DECODER *d)
{
       ilclient_flush_tunnels(d->tunnel, 0);
       ilclient_disable_port_buffers(d->decode, 130, NULL, NULL, NULL);

       ilclient_disable_tunnel(d->tunnel);
       ilclient_disable_tunnel(d->tunnel+1);
       ilclient_disable_tunnel(d->tunnel+2);
       ilclient_teardown_tunnels(d->tunnel);

       ilclient_state_transition(d->list, OMX_StateIdle);
       ilclient_state_transition(d->list, OMX_StateLoaded);

       ilclient_cleanup_components(d->list);

       OMX_Deinit();

       if (d->client) ilclient_destroy(d->client);

       return true;
}
