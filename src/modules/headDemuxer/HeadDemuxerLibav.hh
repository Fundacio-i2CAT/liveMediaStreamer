/*
 *  HeadDemuxerLibav.hh - A libav-based head (source) demuxer
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

#ifndef _HEAD_DEMUXER_LIBAV_HH
#define _HEAD_DEMUXER_LIBAV_HH

extern "C" {
#include <libavformat/avformat.h>
}

#include "../../Filter.hh"

/** Internal description of each of the streams in the currently set URI.
  * It is made public through #BaseFilter::getState() which returns a Json representation.
  * #extradata is passed to the queues built on #HeadDemuxerLibav::allocQueue() as const, so it cannot
  * be modified.
  */
struct DemuxerStreamInfo {
    StreamType type; //!< AUDIO or VIDEO, basically
    uint8_t *extradata; //!< Codec-specific info. This array belongs to this struct, no-one but us should free it.
    int extradata_size; //!< Amount of bytes in #extradata
    int time_base_num; //!< Multiplier for frame timestamps
    int time_base_den; //!< Divider for frame timestamps
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
            int width;
            int height;
        } video;
    };
};

/** Source + Demuxer filter based on libav.
  * Its only configuration is an input URI, which instantiates the adecuate
  * source (file or network) and demuxer.
  * It produces one writer for each stream contained in the muxed file.
  * Use #BaseFilter::getState() to retrieve the description of the streams and their
  * writerId so you can connect further filters (typically, decoders).
  */
class HeadDemuxerLibav : public HeadFilter {

    public:
        HeadDemuxerLibav();
        ~HeadDemuxerLibav();

        /** Returns TRUE if URI could be opened */
        bool setURI(const std::string URI);

    protected:
        virtual bool doProcessFrame(std::map<int, Frame*> &dstFrames);
        virtual FrameQueue *allocQueue(ConnectionData cData);
        virtual void doGetState(Jzon::Object &filterNode);

    private:
        /** URI currently being played */
        std::string uri;

        /** Maps writer IDs to #DemuxerStreamInfo.
         * We use libav stream index as writer ID.
         * The struct is allocated by the demuxer and it belongs to the demuxer,
         * including extradata, even though a pointer to extradata might be
         * shared with the queues. */
        std::map<int, DemuxerStreamInfo*> streams;

        /** Libav media context. Created on #setURI(), destroyed on filter destruction
         * or subsequent #setURI(). */
        AVFormatContext *av_ctx;

        /** Clear all data, close all files */
        void reset();

        /** Convert from Libav SampleFormat enum to ours */
        SampleFmt getSampleFormatFromLibav(AVSampleFormat libavSampleFmt);
};

#endif
