
/*
 *  StreamInfo.hh - Description of the characteristics of a stream
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
 *  Authors: Xavi Artigas <xavier.artigas@i2cat.net>  
 */

#ifndef _STREAMINFO_HH
#define _STREAMINFO_HH

#include "Types.hh"
#include <string.h> // For memcpy

/** Description of each of the streams in the currently set URI.
  * It is made public through #BaseFilter::getState() which returns a Json representation.
  * #extradata is passed to the queues built on #HeadDemuxerLibav::allocQueue() as const, so it cannot
  * be modified.
  */
struct StreamInfo {
    StreamType type; //!< AUDIO or VIDEO, basically
    uint8_t *extradata; //!< Codec-specific info. This array belongs to this struct, no-one but us should free it.
    int extradata_size; //!< Amount of bytes in #extradata
    union {
        /** Audio-specific data */
        struct {
            ACodecType codec;
            unsigned sampleRate;
            unsigned channels;
            SampleFmt sampleFormat;
        } audio;
        /** Video-specific data */
        struct {
            VCodecType codec;
            PixType pixelFormat;
            union {
                struct {
                    bool annexb;
                } h264or5;
            };
        } video;
    };

    void setExtraData(uint8_t *data, int size) {
        if (extradata) {
            delete[] extradata;
            extradata = NULL;
            extradata_size = 0;
        }
        if (data) {
            extradata = new uint8_t[size];
            memcpy (extradata, data, size);
            extradata_size = size;
        }
    }

    void setCodecDefaults() {
        switch (type) {
            case VIDEO:
                break;
            case AUDIO:
                switch (audio.codec) {
                    case OPUS:
                    case AAC:
                    case MP3:
                        audio.sampleFormat = S16;
                        break;
                    case G711:
                        audio.channels = 1;
                        audio.sampleRate = 8000;
                        audio.sampleFormat = U8;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    StreamInfo(StreamType type = ST_NONE, uint8_t *extradata = NULL, int extradata_size = 0) :
        type(type), extradata(extradata), extradata_size(extradata_size) {}


    ~StreamInfo() {
        setExtraData(NULL, 0);
    }
};

#endif
