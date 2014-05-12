/*
 *  Types - AV types
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#ifndef _TYPES_HH
#define _TYPES_HH

#define MAX_CHANNELS 4
#define DEFAULT_HEIGHT 1080
#define DEFAULT_WIDTH 1920
#define BYTES_PER_PIXEL 3
#define FRAMES_H264 100
#define LENGTH_H264 2000
#define FRAMES_OPUS 1000
#define LENGTH_OPUS 2000
#define FRAMES_PCMU 200
#define MAX_SAMPLES_48K 9600 //200ms
#define MAX_SAMPLES_8K 1600 //200ms


enum CodecType {PCMU, OPUS_C, PCM};

enum SampleFmt {S_NONE, U8, S16, S32, FLT, U8P, S16P, S32P, FLTP};

enum VideoType {H264, RGB24, RGB32, YUV422, YUV420, V_NONE};

enum AudioType {
    OPUS, 
    G711, 
    PCMU_2CH_48K_16, 
    PCMU_1CH_48K_16, 
    PCMU_2CH_8K_16,
    A_NONE
};


#endif