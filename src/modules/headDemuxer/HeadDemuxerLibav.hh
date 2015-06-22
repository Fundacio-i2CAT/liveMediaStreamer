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

#include "../../VideoFrame.hh"
#include "../../FrameQueue.hh"
#include "../../Filter.hh"

class HeadDemuxerLibav : public HeadFilter {

    public:
        HeadDemuxerLibav();
        ~HeadDemuxerLibav();

        /* Returns TRUE if URI could be opened */
        bool setURI(const std::string URI);

    protected:
        virtual bool doProcessFrame(Frame *dst);
        virtual FrameQueue *allocQueue(int wId);
        virtual void doGetState(Jzon::Object &filterNode);

    private:
        std::string uri;
};

#endif
