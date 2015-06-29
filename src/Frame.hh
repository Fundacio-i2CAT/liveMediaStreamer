/*
 *  Frame - AV Frame structure
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
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
 *  Authors: David Cassany <david.cassany@i2cat.net>
 *           Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _FRAME_HH
#define _FRAME_HH

#include <sys/time.h>
#include <chrono>
#include "Types.hh"
#include <iostream>

/*! Frame is an abstract class that handles byte array of a video or audio frame
    and frame related information
*/
class Frame {
public:
    /**
    * Creates a frame object
    */
    Frame();
    virtual ~Frame() {};

    /**
    * Set frame presentation time
    * @param system_clock::time_point to set as presentation time
    */
    void setPresentationTime(std::chrono::system_clock::time_point pTime);

    /**
    * Sets a new origin frame time from system_clock::now
    */
    void newOriginTime();

    /**
    * Sets a new origin frame time from input time point
    * @param system_clock::time_point to set as origin
    */
    void setOriginTime(std::chrono::system_clock::time_point orgTime);

    /**
    * Sets a new origin frame time from input time point
    * @param system_clock::time_point to set as origin
    */
    void setDuration(std::chrono::nanoseconds dur);

    /**
    * Sets frame sequence number
    * @param sequence number
    */
    void setSequenceNumber(size_t seqNum);

    std::chrono::system_clock::time_point getPresentationTime() const {return presentationTime;};

    /**
    * Gets origin frame time point
    * @return system_clock::time_point frame origin time
    */
    std::chrono::system_clock::time_point getOriginTime() const {return originTime;};

    /**
    * Gets frame duration
    * @return frame duration as chrono::nanoseconds
    */
    virtual std::chrono::nanoseconds getDuration() const {return duration;};

    /**
    * Gets frame sequence number
    * @return frame sequence number
    */
    size_t getSequenceNumber() const {return sequenceNumber;}

    /**
    * Pure virtual method for getting frame data bytes
    * @return frame data bytes as unsigned char pointer
    */
    virtual unsigned char *getDataBuf() = 0;

    /**
    * Pure virtual method for getting planar frame data bytes
    * @return planar frame data bytes as unsigned char pointer
    */
    virtual unsigned char **getPlanarDataBuf() = 0;

    /**
    * Pure virtual method for getting frame size in bytes
    * @return frame size in bytes
    */
    virtual unsigned int getLength() = 0;

    /**
    * Pure virtual method for getting maximum frame size in bytes
    * @return defined frame size in bytes
    */
    virtual unsigned int getMaxLength() = 0;

    /**
    * Pure virtual method for setting frame size in bytes
    * @param frame size in bytes
    */
    virtual void setLength(unsigned int length) = 0;

    /**
    * Pure virtual method to know if it is a planar frame or not
    * @return true if it is planar, otherwise false
    */
    virtual bool isPlanar() = 0;

    bool getConsumed() const { return consumed; }
    void setConsumed(bool c) { consumed=c; }

protected:
    std::chrono::system_clock::time_point presentationTime;
    std::chrono::system_clock::time_point originTime;
    std::chrono::nanoseconds duration;
    size_t sequenceNumber;
    bool consumed;
};

#endif
