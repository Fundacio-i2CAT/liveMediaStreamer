/*
 *  Utils.hh - Different utils
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _UTILS_HH
#define _UTILS_HH

#include "Types.hh"
#include <string>

#define ID_LENGTH 4
#define BYTE_TO_BIT 8

enum DefinedLogLevel {ERROR, WARNING, DEBUG, INFO};

namespace utils
{
    SampleFmt getSampleFormatFromString(std::string stringSampleFmt);
    ACodecType getCodecFromString(std::string stringCodec);
    FilterType getFilterTypeFromString(std::string stringFilterType);
    TxFormat getTxFormatFromString(std::string stringTxFormat);
    std::string getSampleFormatAsString(SampleFmt sFormat);
    std::string getAudioCodecAsString(ACodecType codec);
    std::string getFilterTypeAsString(FilterType type);
    std::string getWorkerTypeAsString(WorkerType type);
    std::string getTxFormatAsString(TxFormat format);
    std::string randomIdGenerator(unsigned int length);
    int getPayloadFromCodec(std::string codec);


    void errorMsg(std::string msg);
    void warningMsg(std::string msg);
    void infoMsg(std::string msg);
    void debugMsg(std::string msg);

    void setLogLevel(DefinedLogLevel level);
    void printMood(bool mood);
}

#endif
