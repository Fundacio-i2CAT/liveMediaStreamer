/*
 *  Libi2dash - is an ANSI C DASH library in development of ISO/IEC 23009-1
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of libi2dash.
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
 *  Authors:		Martin German <martin.german@i2cat.net>
			Nadim El Taha <nadim.el.taha@i2cat.net>		

 */

#ifndef __CONTEXT__
#define __CONTEXT__

typedef unsigned char byte;
#define NO_TYPE 0
#define VIDEO_TYPE 1
#define AUDIO_TYPE 2
#define AUDIOVIDEO_TYPE 3
#define MAX_MDAT_SAMPLE 65536
#define MAX_DAT 100*1024*1024
//TODO: error negative values
#define I2ERROR_MAX 10
#define I2ERROR_SPS_PPS 9
#define I2ERROR_IS_INTRA 8
#define I2ERROR_DURATION_ZERO 7
#define I2ERROR_DESTINATION_NULL 6
#define I2ERROR_SOURCE_NULL 5
#define I2ERROR_CONTEXT_NULL 4
#define I2ERROR_ISOFF 3
#define I2ERROR_MEDIA_TYPE 2
#define I2ERROR_SIZE_ZERO 1
#define I2OK    0
#define TRUE    1
#define FALSE   0
#define FRAMERATE_PER_CENT 10

#include <netinet/in.h>

typedef struct {
    uint32_t        size;
    uint32_t        duration;
    uint32_t        presentation_timestamp;
    unsigned        key:1;//Flags mp4parser.com
    uint32_t        index;
    uint32_t        decode_timestamp;//not used
} mdat_sample;

typedef struct {
    uint32_t        box_flags;
    mdat_sample     mdat[MAX_MDAT_SAMPLE];
    uint32_t        mdat_sample_length;
    uint32_t        mdat_total_size;
    uint32_t        moof_pos;
    uint32_t        trun_pos;
} i2ctx_sample;

//CONTEXT
typedef struct {
    byte            *pps_sps_data;
    uint32_t        pps_sps_data_length;
    byte            segment_data[MAX_DAT];
    uint32_t        segment_data_size;
    uint32_t        time_base;
    uint32_t        sample_duration;
    uint16_t        width;
    uint16_t        height;
    uint32_t        frame_rate;
    uint32_t        earliest_presentation_time;
    uint32_t        sequence_number;
    uint32_t        current_video_duration;
    i2ctx_sample    *ctxsample;
} i2ctx_video;

typedef struct {
    byte            *aac_data;
    uint32_t        aac_data_length;
    byte            segment_data[MAX_DAT];
    uint32_t        segment_data_size;
    uint32_t        time_base;
    uint32_t        sample_duration;
    uint16_t        channels;
    uint16_t        sample_rate;
    uint16_t        sample_size;
    uint32_t        earliest_presentation_time;
    uint32_t        sequence_number;
    uint32_t        current_audio_duration;
    i2ctx_sample    *ctxsample;
} i2ctx_audio;

typedef struct {
    i2ctx_audio     *ctxaudio;
    i2ctx_video     *ctxvideo;
    uint32_t        duration;
    uint32_t        threshold;
    uint32_t        reference_size;//TODO: refactor
    uint8_t         audio_segment_flag;
} i2ctx;

#endif
