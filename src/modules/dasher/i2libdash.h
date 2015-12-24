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

#ifndef __DASH_LIB__
#define __DASH_LIB__

#include "i2context.h"
#include "i2libisoff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//uint8_t context_initializer(i2ctx **context, uint32_t media_type);

uint8_t generate_context(i2ctx **context, uint32_t media_type); 

uint8_t fill_video_context(i2ctx **context, uint32_t width, uint32_t height, uint32_t t_base);

uint8_t fill_audio_context(i2ctx **context, uint32_t channels, uint32_t sample_rate, uint32_t sample_size, uint32_t t_base, uint32_t sample_duration); 

//TODO: metadata1, 2 and 3 refactor (inside sps)
uint32_t init_video_handler(byte *metadata, uint32_t metadata_size, byte *metadata2, uint32_t metadata2_size, byte *sps_data, uint32_t *sps_size, byte *metadata3, uint32_t metadata3_size, byte *pps_data, uint32_t pps_size, byte *output_data, i2ctx **context);

uint32_t new_init_video_handler(byte *metadata, uint32_t metadata_size, byte *output_data, i2ctx **context);

uint32_t init_audio_handler(byte *input_data, uint32_t size_input, byte *output_data, i2ctx **context);

uint32_t add_sample(byte *input_data, uint32_t size_input, uint32_t duration_sample, uint64_t pts, uint64_t dts, 
                    uint32_t seqNumber, uint32_t media_type, byte *output_data, uint8_t is_intra, i2ctx **context);

uint32_t finish_segment(uint32_t media_type, byte *output_data, i2ctx **context);

void set_segment_duration(uint32_t segment_duration, i2ctx **context);

uint32_t get_segment_duration(i2ctx *context);

void set_sample_rate(uint32_t sample_rate, i2ctx **context);

uint32_t get_sample_rate(i2ctx *context);

uint32_t generate_video_segment(uint8_t nextFrameIsIntra, uint32_t nextFramePts, byte *output_data, 
                                 i2ctx **context, uint64_t* segmentTimestamp, uint32_t* segmentDuration);

uint32_t generate_audio_segment(byte *output_data, i2ctx **context, uint64_t* segmentTimestamp, uint32_t* segmentDuration);

uint32_t force_generate_audio_segment(byte *output_data, i2ctx **context, uint64_t* segmentTimestamp, uint32_t* segmentDuration);

uint32_t add_video_sample(byte *input_data, uint32_t input_data_length, uint64_t pts, 
                           uint64_t dts, uint32_t seqNumber, uint8_t is_intra, i2ctx **context);

uint32_t add_audio_sample(byte *input_data, uint32_t input_data_length, uint32_t sample_duration, 
                          uint64_t pts, uint64_t dts, uint32_t seqNumber, i2ctx **context);

void context_refresh(i2ctx **context, uint32_t media_type);


#endif

