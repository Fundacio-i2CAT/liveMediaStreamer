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
 *            David Cassany <david.cassany@i2cat.net>
 */

#include "Utils.hh"

#include <algorithm>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <sys/time.h>
#include <random>

using namespace log4cplus;

static bool logConfigured = false;

namespace utils
{
    void configureLog(){
        BasicConfigurator config;
        config.configure();

        logConfigured = true;
    }

    SampleFmt getSampleFormatFromString(std::string stringSampleFmt)
    {
        SampleFmt sampleFormat;

        if (stringSampleFmt.compare("u8") == 0) {
            sampleFormat = U8;
        } else if (stringSampleFmt.compare("u8p") == 0) {
            sampleFormat = U8P;
        }  else if (stringSampleFmt.compare("s16") == 0) {
            sampleFormat = S16;
        }  else if (stringSampleFmt.compare("s16p") == 0) {
            sampleFormat = S16P;
        }  else if (stringSampleFmt.compare("flt") == 0) {
            sampleFormat = FLT;
        }  else if (stringSampleFmt.compare("fltp") == 0) {
            sampleFormat = FLTP;
        }  else {
            sampleFormat = S_NONE;
        }

        return sampleFormat;
    }

    ACodecType getCodecFromString(std::string stringCodec)
    {
        ACodecType codec;
        if (stringCodec.compare("g711") == 0) {
            codec = G711;
        } else if (stringCodec.compare("pcmu") == 0) {
            codec = PCMU;
        }  else if (stringCodec.compare("opus") == 0) {
            codec = OPUS;
        }  else if (stringCodec.compare("pcm") == 0) {
            codec = PCM;
        }  else if (stringCodec.compare("aac") == 0) {
            codec = AAC;
        }  else if (stringCodec.compare("mp3") == 0) {
            codec = MP3;
        }  else {
            codec = AC_NONE;
        }

        return codec;
    }

    std::string getAudioCodecAsString(ACodecType codec)
    {
       std::string stringCodec;

        switch(codec) {
            case G711:
                stringCodec = "g711";
                break;
            case PCMU:
                stringCodec = "pcmu";
                break;
            case OPUS:
                stringCodec = "opus";
                break;
            case PCM:
                stringCodec = "pcm";
                break;
            case AAC:
                stringCodec = "aac";
                break;
            case MP3:
                stringCodec = "mp3";
                break;
            default:
                stringCodec = "";
                break;
        }

        return stringCodec;
    }

    std::string getFilterTypeAsString(FilterType type)
    {
        std::string stringType;

        switch(type) {
            case VIDEO_DECODER:
                stringType = "videoDecoder";
                break;
            case VIDEO_ENCODER:
                stringType = "videoEncoder";
                break;
            case VIDEO_MIXER:
                stringType = "videoMixer";
                break;
            case VIDEO_RESAMPLER:
                stringType = "videoResampler";
                break;
            case AUDIO_DECODER:
                stringType = "audioDecoder";
                break;
            case AUDIO_ENCODER:
                stringType = "audioEncoder";
                break;
            case AUDIO_MIXER:
                stringType = "audioMixer";
                break;
            case RECEIVER:
                stringType = "receiver";
                break;
            case TRANSMITTER:
                stringType = "transmitter";
                break;
            default:
                stringType = "";
                break;
        }

        return stringType;
    }

    FilterType getFilterTypeFromString(std::string stringFilterType)
    {
        FilterType fType;

        if (stringFilterType.compare("videoDecoder") == 0) {
           fType = VIDEO_DECODER;
        } else if (stringFilterType.compare("videoEncoder") == 0) {
           fType = VIDEO_ENCODER;
        }  else if (stringFilterType.compare("videoMixer") == 0) {
           fType = VIDEO_MIXER;
        }  else if (stringFilterType.compare("videoResampler") == 0) {
           fType = VIDEO_RESAMPLER;
        }  else if (stringFilterType.compare("audioDecoder") == 0) {
           fType = AUDIO_DECODER;
        }  else if (stringFilterType.compare("audioEncoder") == 0) {
           fType = AUDIO_ENCODER;
        }  else if (stringFilterType.compare("audioMixer") == 0) {
           fType = AUDIO_MIXER;
        }  else if (stringFilterType.compare("receiver") == 0) {
           fType = RECEIVER;
        }  else if (stringFilterType.compare("transmitter") == 0) {
           fType = TRANSMITTER;
        }  else {
           fType = FT_NONE;
        }

        return fType;
    }

    std::string getSampleFormatAsString(SampleFmt sFormat)
    {
        std::string stringFormat;

        switch(sFormat) {
            case U8:
                stringFormat = "u8";
                break;
            case S16:
                stringFormat = "s16";
                break;
            case FLT:
                stringFormat = "flt";
                break;
            case U8P:
                stringFormat = "u8p";
                break;
            case S16P:
                stringFormat = "s16p";
                break;
            case FLTP:
                stringFormat = "fltp";
                break;
            default:
                stringFormat = "";
                break;
        }

        return stringFormat;

    }

    std::string getWorkerTypeAsString(WorkerType type)
    {
        std::string stringWorker;

        switch(type) {
            case LIVEMEDIA:
                stringWorker = "livemedia";
                break;
            case WORKER:
                stringWorker = "master";
                break;
//            case SLAVE:
//                stringWorker = "slave";
//                break;
            default:
                stringWorker = "";
                break;
        }

        return stringWorker;
    }

    TxFormat getTxFormatFromString(std::string stringTxFormat)
    {
        TxFormat format;

        if (stringTxFormat.compare("std") == 0) {
           format = STD_RTP;
        } else if (stringTxFormat.compare("ultragrid") == 0) {
           format = ULTRAGRID;
        }  else if (stringTxFormat.compare("mpegts") == 0) {
           format = MPEGTS;
        }  else {
           format = TX_NONE;
        }

        return format;
    }

    std::string getTxFormatAsString(TxFormat format)
    {
        std::string stringFormat;

        switch(format) {
            case STD_RTP:
                stringFormat = "std";
                break;
            case ULTRAGRID:
                stringFormat = "ultragrid";
                break;
            case MPEGTS:
                stringFormat = "mpegts";
                break;
            default:
                stringFormat = "";
                break;
        }

        return stringFormat;
    }

    char randAlphaNum()
    {
        static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

        return alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    std::string randomIdGenerator(unsigned int length)
    {
        std::string id(length,0);
        std::generate_n(id.begin(), length, randAlphaNum);
        return id;
    }

    int getPayloadFromCodec(std::string codec)
    {
        int payload;

        if (codec.compare("pcmu") == 0 ||
            codec.compare("opus") == 0 ||
            codec.compare("pcm") == 0 ||
            codec.compare("mpeg4-generic") == 0) {
            payload = 97;
        } else if (codec.compare("mp3") == 0) {
            payload = 14;
        } else if (codec.compare("H264") == 0) {
            payload = 96;
        } else {
            payload = -1;
        }

        return payload;
    }

    void setLogLevel(DefinedLogLevel level)
    {
        if (!logConfigured){
            configureLog();
        }

        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("main"));

        switch(level) {
            case ERROR:
                logger.setLogLevel(ERROR_LOG_LEVEL);
                break;
            case WARNING:
                logger.setLogLevel(WARN_LOG_LEVEL);
                break;
            case DEBUG:
                logger.setLogLevel(DEBUG_LOG_LEVEL);
                break;
            case INFO:
                logger.setLogLevel(INFO_LOG_LEVEL);
                break;
        }
    }

    void warningMsg(std::string msg)
    {
        if (!logConfigured){
            configureLog();
        }

        if (msg.empty()){
            return;
        }

        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("main"));
        LOG4CPLUS_WARN(logger, msg);
    }

    void debugMsg(std::string msg)
    {
        if (!logConfigured){
            configureLog();
        }

        if (msg.empty()){
            return;
        }

        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("main"));
        LOG4CPLUS_DEBUG(logger, msg);
    }

    void errorMsg(std::string msg)
    {
        if (!logConfigured){
            configureLog();
        }

        if (msg.empty()){
            return;
        }

        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("main"));
        LOG4CPLUS_ERROR(logger, msg);
    }

    void infoMsg(std::string msg)
    {
        if (!logConfigured){
            configureLog();
        }

        if (msg.empty()){
            return;
        }

        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("main"));
        LOG4CPLUS_INFO(logger, msg);
    }
}
