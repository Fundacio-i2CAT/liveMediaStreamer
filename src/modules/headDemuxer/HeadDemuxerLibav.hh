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
#include <libavutil/avutil.h>
}

#include "../../Filter.hh"
#include "../../StreamInfo.hh"
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

        /**Sets and tries to open the URI provided
         * @returns TRUE if URI could be opened 
         */
        bool setURI(const std::string URI);

    protected:
        virtual bool doProcessFrame(std::map<int, Frame*> &dstFrames);
        virtual FrameQueue *allocQueue(ConnectionData cData);
        virtual void doGetState(Jzon::Object &filterNode);

    private:
        //NOTE: There is no need of specific writer configuration
        bool specificWriterConfig(int /*writerID*/) {return true;};
        bool specificWriterDelete(int /*writerID*/) {return true;};
        
        /** Maps writer IDs to the private stream information */
        struct PrivateStreamInfo {
            /** Time base needed to convert libav PTS to seconds */
            double streamTimeBase;
            /** Whether the input needs to be parsed to split packets into multiple frames */
            bool needsFraming;
            /* Whether the input is H264 in AVCC or Annex B format */
            bool isAnnexB;
            /* Initialized to zero, it keeps the last valid pts, in order to be used 
             * in case an AV_NOPTS_VALUE is found in av_pkt*/
            int64_t lastPTS;
            /* Last system time used in a frame*/
            std::chrono::microseconds lastSTime;
        };

        /** URI currently being played */
        std::string uri;

        /** Maps writer IDs to #StreamInfo.
         * We use libav stream index as writer ID.
         * The struct is allocated by the demuxer and it belongs to the demuxer,
         * including extradata, even though a pointer to extradata might be
         * shared with the queues. */
        std::map<int, StreamInfo*> outputStreamInfos;

        /** Stream information needed for demuxer operation, but not public. */
        std::map<int, PrivateStreamInfo*> privateStreamInfos;

        /** Libav media context. Created on #setURI(), destroyed on filter destruction
         * or subsequent #setURI(). */
        AVFormatContext *av_ctx;
        /** Libav filter required to convert H264 AVCC to AnnexB */
        AVBitStreamFilterContext *av_filter_annexb;
        /** Packet being processed, in case it contains more than one NALU and it needs
         * to be persisted among multiple calls to doProcessFrame. */
        AVPacket av_pkt;

        /** Working buffer. It might point inside av_pkt, or to a temp buffer needed
         * when converting H264 from AVCC to AnnexB */
        uint8_t *buffer;
        int bufferSize;
        int bufferOffset;

        /** Clear all data, close all files */
        void reset();

        /** Initialize its events */
        void initializeEventMap();

        /** This event sets the demuxer's input URI */
        bool configureEvent(Jzon::Node* params);

        /** Convert from Libav SampleFormat enum to ours */
        SampleFmt getSampleFormatFromLibav(AVSampleFormat libavSampleFmt);
};

#endif
