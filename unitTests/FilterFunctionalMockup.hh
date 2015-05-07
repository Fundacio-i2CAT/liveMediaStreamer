/*
 *  FilterFunctionalMockup - OneToOne Filter scenario test
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
 *  Authors:    David Cassany <david.cassany@i2cat.net>
 *
 */

#ifndef _FILTER_FUNCTIONAL_MOCKUP_HH
#define _FILTER_FUNCTIONAL_MOCKUP_HH

#include <chrono>

#include "Filter.hh"
#include "VideoFrame.hh"
#include "FilterMockup.hh"


class OneToOneVideoScenarioMockup {
public: 
    OneToOneVideoScenarioMockup(OneToOneFilter* fToTest, VCodecType c, PixType pix = P_NONE): filterToTest(fToTest){
        headF = new VideoHeadFilterMockup(c, pix);
        tailF = new VideoTailFilterMockup();
    };
    
    bool connectFilter(){
        if (filterToTest == NULL){
            return false;
        }
        
        if (! headF->connectOneToOne(filterToTest)){
            return false;
        }
        
        if (! filterToTest->connectOneToOne(tailF)){
            return false;
        }
        
        return true;
    };
    
    std::chrono::nanoseconds processFrame(InterleavedVideoFrame* srcFrame, InterleavedVideoFrame* filteredFrame){
        std::chrono::nanoseconds ret;
        
        if (! headF->inject(srcFrame)){
            return std::chrono::nanoseconds(0);
        }
        headF->processFrame();
        ret = filterToTest->processFrame();
        tailF->processFrame();
        filteredFrame = tailF->extract();
        
        if (! filteredFrame){
            return std::chrono::nanoseconds(0);
        }
        
        return ret;
    }
    
private:
    VideoHeadFilterMockup *headF;
    VideoTailFilterMockup *tailF;
    OneToOneFilter* filterToTest;
}




#endif