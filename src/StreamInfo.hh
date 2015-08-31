
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

/** Description of a stream. It is created by filters and shared with queues through const pointers, so
 * only the filter can destroy it. #extradata belongs to this struct, do not free.
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
                    /** If true, bitstream is in Annex B format, so each NALU is prefixed with a
                     * startcode (00 00 00 01 or 00 00 01).
                     * If false, bitstream is in AVCC format, there are no startcodes. */
                    bool annexb;
                    /** If true, each data buffer (#Frame, in LMS) contains a single NALU.
                     * Otherwise, multiple NALUs can be present in a single buffer and parsing might
                     * be required.
                     */
                    bool framed;
                } h264or5;
            };
        } video;
    };

    /** Preferred method to set the #extradata, since it takes care of disposing of previous values.
     * Pass NULL to free current #extradata. */
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

    /** Sets default values for some attributes, based on the #type and the specific codec.
     * These default values where in use throughout LMS before the introduction of #StreamInfo. */
    void setCodecDefaults() {
        switch (type) {
            case VIDEO:
                video.h264or5.annexb = false;
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

    /** Just provide as much information as you want, the rest is initialized to sane defaults. */
    StreamInfo(StreamType type = ST_NONE, uint8_t *extradata = NULL, int extradata_size = 0) :
        type(type), extradata(extradata), extradata_size(extradata_size) {
            /* These are the default values that were previously used (or assumed) in LMS. */
            switch (type) {
                case AUDIO:
                    audio.codec = AC_NONE;
                    audio.sampleRate = 0;
                    audio.channels = 0;
                    audio.sampleFormat = S_NONE;
                    break;
                case VIDEO:
                    video.codec = VC_NONE;
                    video.pixelFormat = P_NONE;
                    video.h264or5.annexb = false;
                    video.h264or5.framed = true;
                    break;
                default:
                    break;
            }
        }


    ~StreamInfo() {
        setExtraData(NULL, 0);
    }
};

#endif
