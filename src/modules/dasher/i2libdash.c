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

#include "i2libdash.h"

// PRIVATE FUNCTIONS DECLARATION
void context_refresh(i2ctx **context, uint32_t media_type);

void audio_context_initializer(i2ctx **context);

void audio_sample_context_initializer(i2ctx_audio **ctxAudio);

void video_context_initializer(i2ctx **context);

void video_sample_context_initializer(i2ctx_video **ctxVideo);

uint8_t is_key_frame(byte *input_data, uint32_t size_input);

void set_segment_duration(uint32_t segment_duration, i2ctx **context){
    (*context)->duration = segment_duration;
}

uint32_t get_segment_duration(i2ctx *context){
    return context->duration;
}

uint32_t get_frame_rate(i2ctx *context){
    return context->ctxvideo->frame_rate;
}

void set_sample_rate(uint32_t sample_rate, i2ctx **context){
    (*context)->ctxaudio->sample_rate = sample_rate;
}

uint32_t get_sample_rate(i2ctx *context){
    return context->ctxaudio->sample_rate;
}

void audio_context_initializer(i2ctx **context) {
    (*context)->ctxaudio = (i2ctx_audio *) malloc(sizeof(i2ctx_audio));
    i2ctx_audio *ctxAudio = (*context)->ctxaudio;

    ctxAudio->aac_data_length = 0;
    ctxAudio->segment_data_size = 0;
    ctxAudio->channels = 0;
    ctxAudio->sample_rate = 0;
    ctxAudio->sample_size = 0;
    ctxAudio->time_base = 0;
    ctxAudio->sequence_number = 0;
    ctxAudio->earliest_presentation_time = 0;
    ctxAudio->current_audio_duration = 0;

    audio_sample_context_initializer(&ctxAudio);
}

void audio_sample_context_initializer(i2ctx_audio **ctxAudio) {
    (*ctxAudio)->ctxsample = (i2ctx_sample *) malloc(sizeof(i2ctx_sample));
    i2ctx_sample *ctxASample = (*ctxAudio)->ctxsample;

    ctxASample->box_flags = 769;
    ctxASample->mdat_sample_length = 0;
    ctxASample->mdat_total_size = 0;
    ctxASample->moof_pos = 0;
    ctxASample->trun_pos = 0;
}

void video_context_initializer(i2ctx **context) {
    (*context)->ctxvideo = (i2ctx_video *) malloc(sizeof(i2ctx_video));
    i2ctx_video *ctxVideo = (*context)->ctxvideo;

    ctxVideo->pps_sps_data_length = 0;
    ctxVideo->segment_data_size = 0;
    ctxVideo->width = 0;
    ctxVideo->height = 0;
    ctxVideo->frame_rate = 0;
    ctxVideo->time_base = 0;
    ctxVideo->earliest_presentation_time = 0;
    ctxVideo->sequence_number = 0;
    ctxVideo->current_video_duration = 0;

    video_sample_context_initializer(&ctxVideo);
}

void context_refresh(i2ctx **context, uint32_t media_type) {
    if ((media_type == VIDEO_TYPE) || (media_type == AUDIOVIDEO_TYPE)) {
        (*context)->ctxvideo->earliest_presentation_time = 0;
        (*context)->ctxvideo->sequence_number = 0;
        (*context)->ctxvideo->current_video_duration = 0;
        (*context)->ctxvideo->segment_data_size = 0;
        (*context)->ctxvideo->ctxsample->mdat_sample_length = 0;
        (*context)->ctxvideo->ctxsample->mdat_total_size = 0;
        (*context)->ctxvideo->ctxsample->moof_pos = 0;
        (*context)->ctxvideo->ctxsample->trun_pos = 0;
    }
    if ((media_type == AUDIO_TYPE) || (media_type == AUDIOVIDEO_TYPE)) {
        (*context)->ctxaudio->earliest_presentation_time = 0;
        (*context)->ctxaudio->sequence_number = 0;
        (*context)->ctxaudio->current_audio_duration = 0;
        (*context)->ctxaudio->segment_data_size = 0;
        (*context)->ctxaudio->ctxsample->mdat_sample_length = 0;
        (*context)->ctxaudio->ctxsample->mdat_total_size = 0;
        (*context)->ctxaudio->ctxsample->moof_pos = 0;
        (*context)->ctxaudio->ctxsample->trun_pos = 0;
    }
}

void video_sample_context_initializer(i2ctx_video **ctxVideo) {
    (*ctxVideo)->ctxsample = (i2ctx_sample *) malloc(sizeof(i2ctx_sample)); 
    i2ctx_sample *ctxVSample = (*ctxVideo)->ctxsample;

    ctxVSample->box_flags = 3841;
    ctxVSample->mdat_sample_length = 0;
    ctxVSample->mdat_total_size = 0;
    ctxVSample->moof_pos = 0;
    ctxVSample->trun_pos = 0;
}

uint8_t generate_context(i2ctx **context, uint32_t media_type) 
{
    if ((media_type != VIDEO_TYPE) && (media_type != AUDIO_TYPE) && (media_type != AUDIOVIDEO_TYPE)) {
        (*context) = NULL;
        return I2ERROR_MEDIA_TYPE;
    }

    *context = (i2ctx *) malloc(sizeof(i2ctx));
    (*context)->reference_size = 0;

    if ((media_type == VIDEO_TYPE) || (media_type == AUDIOVIDEO_TYPE)) {
        video_context_initializer(context);
    } else {
        (*context)->ctxvideo = NULL;
    }

    if ((media_type == AUDIO_TYPE) || (media_type == AUDIOVIDEO_TYPE)) {
        audio_context_initializer(context);
    } else {
        (*context)->ctxaudio = NULL;
    }

    return I2OK;
}

uint8_t fill_video_context(i2ctx **context, uint32_t width, uint32_t height, uint32_t framerate, uint32_t t_base, uint32_t sample_duration) 
{
    uint32_t fps_per_cent = 0;

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }

    if ((*context)->ctxvideo == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }

    (*context)->ctxvideo->width = width;
    (*context)->ctxvideo->height = height;
    (*context)->ctxvideo->frame_rate = framerate;
    (*context)->ctxvideo->time_base = t_base;
    (*context)->ctxvideo->sample_duration = sample_duration;

    fps_per_cent = ((*context)->ctxvideo->frame_rate * FRAMERATE_PER_CENT)/100;
    // Threshold: 1/fps * %fps * 1000
    (*context)->threshold = ((*context)->ctxvideo->time_base * fps_per_cent)/(((*context)->ctxvideo->frame_rate)); 

    return I2OK;
}

uint8_t fill_audio_context(i2ctx **context, uint32_t channels, uint32_t sample_rate, uint32_t sample_size, uint32_t t_base, uint32_t sample_duration) 
{
    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }

    if ((*context)->ctxaudio == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }

    (*context)->ctxaudio->channels = channels;
    (*context)->ctxaudio->sample_rate = sample_rate;
    (*context)->ctxaudio->sample_size = sample_size;
    (*context)->ctxaudio->time_base = t_base;
    (*context)->ctxaudio->sample_duration = sample_duration;

    return I2OK;
}

uint32_t new_init_video_handler(byte *metadata, uint32_t metadata_size, byte *output_data, i2ctx **context) 
{
    uint32_t initSize;

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }

    if (!output_data) {
        return I2ERROR_DESTINATION_NULL;
    }

    if (!metadata) {
        return I2ERROR_SOURCE_NULL;
    }

    if (metadata_size < 1) {
        return I2ERROR_SIZE_ZERO;
    }

    if ((*context)->ctxvideo->width == 0 || (*context)->ctxvideo->height == 0 || (*context)->ctxvideo->frame_rate == 0) {
        return I2ERROR_SIZE_ZERO;
    }

    initSize = initVideoGenerator(metadata, metadata_size, output_data, context);

    return initSize;
}

uint32_t init_video_handler(byte *metadata, uint32_t metadata_size, byte *metadata2, uint32_t metadata2_size, byte *sps_data, uint32_t *sps_size, byte *metadata3, uint32_t metadata3_size, byte *pps_data, uint32_t pps_size, byte *output_data, i2ctx **context) {
    uint32_t initVideo, count, sps_pps_data_length;
    uint16_t pps16, sps16, hton_sps_size, hton_pps_size;
    byte *sps_pps_data;
    uint32_t sps_s = *sps_size;
    /*uint32_t width, height;   
    nalHeader headers;*/

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }
    if (output_data == NULL) {
        return I2ERROR_DESTINATION_NULL;
    }
    if ((metadata == NULL) || (metadata2 == NULL) || (metadata3 == NULL) || (sps_data == NULL) || (pps_data == NULL)) {
        return I2ERROR_SOURCE_NULL;
    }
    if ((metadata_size < 1) || (metadata2_size < 1) || (metadata3_size < 1) || ((*sps_size) < 1) || (pps_size < 1)) {
        return I2ERROR_SIZE_ZERO;
    }

    count = 0;

    sps_pps_data = (byte *) malloc(metadata_size + metadata2_size + 2 + sps_s + metadata3_size + 2 + pps_size);

    // Metadata
    memcpy(sps_pps_data + count, metadata, metadata_size);
    count = count + metadata_size;
    
    // Metadata2
    memcpy(sps_pps_data + count, metadata2, metadata2_size);
    count = count + metadata2_size;
    
    // Size SPS
    sps16 = sps_s;
    hton_sps_size = htons(sps16);
    memcpy(sps_pps_data + count, &hton_sps_size, 2);
    count = count + 2;
   
    // SPS
    memcpy(sps_pps_data + count, sps_data, sps_s);
    count = count + sps_s;
   
    // Metadata3
    memcpy(sps_pps_data + count, metadata3, metadata3_size);
    count = count + metadata3_size;
   
    // Size PPS
    pps16 = pps_size;
    hton_pps_size = htons(pps16);
    memcpy(sps_pps_data + count, &hton_pps_size, 2);
    count = count + 2;
    
    // PPS
    memcpy(sps_pps_data + count, pps_data, pps_size);
    count = count + pps_size;

    sps_pps_data_length = count;
    
    initVideo = initVideoGenerator(sps_pps_data, sps_pps_data_length, output_data, context);

    return initVideo;
}

uint32_t init_audio_handler(byte *input_data, uint32_t size_input, byte *output_data, i2ctx **context) {
    uint32_t initAudio;
    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }
    if (output_data == NULL) {
        return I2ERROR_DESTINATION_NULL;
    }
    if (input_data == NULL) {
        return I2ERROR_SOURCE_NULL;
    }   
    if (size_input < 1) {
        return I2ERROR_SIZE_ZERO;
    }
    initAudio =  initAudioGenerator(input_data, size_input, output_data, context);

    return initAudio;
}

uint32_t generate_video_segment(uint8_t nextFrameIsIntra, byte *output_data, i2ctx **context, uint32_t* segmentTimestamp)
{
    uint32_t segDataLength;

    segDataLength = 0;

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }
    
    if (output_data == NULL) {
        return I2ERROR_DESTINATION_NULL;
    }

    if ((nextFrameIsIntra != TRUE) && (nextFrameIsIntra != FALSE)) {
        return I2ERROR_IS_INTRA;
    }

    if ((nextFrameIsIntra == TRUE) && ((((*context)->duration - (*context)->threshold)) <= ((*context)->ctxvideo->current_video_duration))) {
        segDataLength = segmentGenerator((*context)->ctxvideo->segment_data, (*context)->ctxvideo->segment_data_size, output_data, VIDEO_TYPE, context);

        if (segDataLength <= I2ERROR_MAX) {
            return segDataLength;
        }

        *segmentTimestamp = (*context)->ctxvideo->earliest_presentation_time;
        context_refresh(context, VIDEO_TYPE);
    }

    return segDataLength;
}

uint32_t generate_audio_segment(byte *output_data, i2ctx **context, uint32_t* segmentTimestamp)
{
    uint32_t segDataLength;

    segDataLength = 0;

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }
    
    if (output_data == NULL) {
        return I2ERROR_DESTINATION_NULL;
    }

    if ((*context)->duration <= (*context)->ctxaudio->current_audio_duration) { 

        segDataLength = segmentGenerator((*context)->ctxaudio->segment_data, (*context)->ctxaudio->segment_data_size, output_data, AUDIO_TYPE, context);

        if (segDataLength <= I2ERROR_MAX) {
            return segDataLength;
        }

        *segmentTimestamp = (*context)->ctxaudio->earliest_presentation_time;
        context_refresh(context, AUDIO_TYPE);
    }

    return segDataLength;
}

uint32_t add_video_sample(byte *input_data, uint32_t input_data_length, uint32_t sample_duration, 
                          uint32_t pts, uint32_t dts, uint32_t seqNumber, uint8_t is_intra, i2ctx **context)
{
    uint32_t samp_len;

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }
    
    if (input_data == NULL) {
        return I2ERROR_SOURCE_NULL;
    }

    if (input_data_length < 1) {
        return I2ERROR_SIZE_ZERO;
    }

    if (sample_duration < 1) {
        return I2ERROR_DURATION_ZERO;
    }

    if ((is_intra != TRUE) && (is_intra != FALSE)) {
        return I2ERROR_IS_INTRA;
    }


    i2ctx_sample *ctxSample = (*context)->ctxvideo->ctxsample;

    if (ctxSample->mdat_sample_length == 0 && is_intra != TRUE) {
        return I2ERROR_IS_INTRA;
    )

    // Add segment data
    memcpy((*context)->ctxvideo->segment_data + (*context)->ctxvideo->segment_data_size, input_data, input_data_length);
    (*context)->ctxvideo->segment_data_size += input_data_length;

    if(ctxSample->mdat_sample_length == 0) {
        (*context)->ctxvideo->earliest_presentation_time = pts;
        (*context)->ctxvideo->sequence_number = seqNumber;
    }

    (*context)->ctxvideo->current_video_duration += sample_duration;

    // Add metadata
    samp_len = ctxSample->mdat_sample_length;
    ctxSample->mdat[samp_len].size = input_data_length;
    ctxSample->mdat[samp_len].duration = sample_duration;
    ctxSample->mdat[samp_len].presentation_timestamp = pts;
    ctxSample->mdat[samp_len].decode_timestamp = dts;
    ctxSample->mdat[samp_len].key = is_intra;
    ctxSample->mdat_sample_length++;

    return I2OK;
}

uint32_t add_audio_sample(byte *input_data, uint32_t input_data_length, uint32_t sample_duration, 
                          uint32_t pts, uint32_t dts, uint32_t seqNumber, i2ctx **context)
{
    uint32_t samp_len;

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }
    if (input_data == NULL) {
        return I2ERROR_SOURCE_NULL;
    }   
    if (input_data_length < 1) {
        return I2ERROR_SIZE_ZERO;
    }
    if (sample_duration < 1) {
        return I2ERROR_DURATION_ZERO;
    }

    // Add sample or Init new segmentation
    i2ctx_sample *ctxSample = (*context)->ctxaudio->ctxsample;
    
    // Add segment data
    memcpy((*context)->ctxaudio->segment_data + (*context)->ctxaudio->segment_data_size, input_data, input_data_length);
    (*context)->ctxaudio->segment_data_size += input_data_length;
    
    if(ctxSample->mdat_sample_length == 0) {
        (*context)->ctxaudio->earliest_presentation_time = pts;
        (*context)->ctxaudio->sequence_number = seqNumber;
    }

    (*context)->ctxaudio->current_audio_duration += sample_duration;

    // Add metadata
    samp_len = ctxSample->mdat_sample_length;
    ctxSample->mdat[samp_len].size = input_data_length;
    ctxSample->mdat[samp_len].duration = sample_duration;
    ctxSample->mdat[samp_len].presentation_timestamp = pts;
    ctxSample->mdat[samp_len].decode_timestamp = dts;
    ctxSample->mdat[samp_len].key = 1;
    ctxSample->mdat_sample_length++;

    return I2OK;
}

uint32_t add_sample(byte *input_data, uint32_t size_input, uint32_t duration_sample, uint32_t pts, uint32_t dts, 
                    uint32_t seqNumber, uint32_t media_type, byte *output_data, uint8_t is_intra, i2ctx **context) 
{
    uint32_t seg_gen, samp_len;

    seg_gen = 0;

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }
    if (output_data == NULL) {
        return I2ERROR_DESTINATION_NULL;
    }
    if (input_data == NULL) {
        return I2ERROR_SOURCE_NULL;
    }   
    if (size_input < 1) {
        return I2ERROR_SIZE_ZERO;
    }
    if (duration_sample < 1) {
        return I2ERROR_DURATION_ZERO;
    }
    if ((is_intra != TRUE) && (is_intra != FALSE)) {
        return I2ERROR_IS_INTRA;
    }

    // TODO AUDIOVIDEO_TYPE
    if ((media_type != AUDIO_TYPE) && (media_type != VIDEO_TYPE)) {
        return I2ERROR_MEDIA_TYPE;
    }

    if (media_type == VIDEO_TYPE) {
        seg_gen = I2OK;
        if ((is_intra == TRUE) && ((((*context)->duration) - ((*context)->threshold)) <= ((*context)->ctxvideo->current_video_duration))) {
            seg_gen = segmentGenerator((*context)->ctxvideo->segment_data, (*context)->ctxvideo->segment_data_size, output_data, VIDEO_TYPE, context);
            if (seg_gen > I2ERROR_MAX)
                context_refresh(context, VIDEO_TYPE);
        }
        
        // Add sample or Init new segmentation
        if ((seg_gen == I2OK) || (seg_gen > I2ERROR_MAX)) {
            i2ctx_sample *ctxSample = (*context)->ctxvideo->ctxsample;
            
            // Add segment data
            memcpy((*context)->ctxvideo->segment_data + (*context)->ctxvideo->segment_data_size, input_data, size_input);
            (*context)->ctxvideo->segment_data_size += size_input;

            if(ctxSample->mdat_sample_length == 0) {
                (*context)->ctxvideo->earliest_presentation_time = pts;
                (*context)->ctxvideo->sequence_number = seqNumber;
            }

            (*context)->ctxvideo->current_video_duration += duration_sample;

            // Add metadata
            samp_len = ctxSample->mdat_sample_length;
            ctxSample->mdat[samp_len].size = size_input;
            ctxSample->mdat[samp_len].duration = duration_sample;
            ctxSample->mdat[samp_len].presentation_timestamp = pts;
            ctxSample->mdat[samp_len].decode_timestamp = dts;
            ctxSample->mdat[samp_len].key = is_intra;
            ctxSample->mdat_sample_length++;
        }

    } else if(media_type == AUDIO_TYPE) {
        seg_gen = I2OK;
        
        // Close segmentation
        if ((((*context)->ctxvideo != NULL) && (is_intra == TRUE)) || (((*context)->ctxvideo == NULL) && (((*context)->duration) <= ((*context)->ctxaudio->current_audio_duration)))) { 
            seg_gen = segmentGenerator((*context)->ctxaudio->segment_data, (*context)->ctxaudio->segment_data_size, output_data, AUDIO_TYPE, context);          
            if (seg_gen > I2ERROR_MAX)
                context_refresh(context, AUDIO_TYPE);
        }
        
        // Add sample or Init new segmentation
        if ((seg_gen == I2OK) || (seg_gen > I2ERROR_MAX)) {
            i2ctx_sample *ctxSample = (*context)->ctxaudio->ctxsample;
            
            // Add segment data
            memcpy((*context)->ctxaudio->segment_data + (*context)->ctxaudio->segment_data_size, input_data, size_input);
            (*context)->ctxaudio->segment_data_size += size_input;
            
            if(ctxSample->mdat_sample_length == 0) {
                (*context)->ctxaudio->earliest_presentation_time = pts;
                (*context)->ctxaudio->sequence_number = seqNumber;
            }

            (*context)->ctxaudio->current_audio_duration += duration_sample;

            // Add metadata
            samp_len = ctxSample->mdat_sample_length;
            ctxSample->mdat[samp_len].size = size_input;
            ctxSample->mdat[samp_len].duration = duration_sample;
            ctxSample->mdat[samp_len].presentation_timestamp = pts;
            ctxSample->mdat[samp_len].decode_timestamp = dts;
            ctxSample->mdat[samp_len].key = is_intra;
            ctxSample->mdat_sample_length++;
        }
    }

    return seg_gen;
}

uint32_t finish_segment(uint32_t media_type, byte *output_data, i2ctx **context) {
    uint32_t seg_gen;
    seg_gen = 0;

    if ((*context) == NULL) {
        return I2ERROR_CONTEXT_NULL;
    }
    if (output_data == NULL) {
        return I2ERROR_DESTINATION_NULL;
    }
    if ((media_type != VIDEO_TYPE) && (media_type != AUDIO_TYPE)) {
        return I2ERROR_MEDIA_TYPE;
    }
    
    if (media_type == VIDEO_TYPE) {
        seg_gen = I2OK;
        
        seg_gen = segmentGenerator((*context)->ctxvideo->segment_data, (*context)->ctxvideo->segment_data_size, output_data, VIDEO_TYPE, context);
        if ((seg_gen == I2OK) || (seg_gen > I2ERROR_MAX))
            context_refresh(context, VIDEO_TYPE);
    } else if(media_type == AUDIO_TYPE) {
        seg_gen = I2OK;
        
        seg_gen = segmentGenerator((*context)->ctxaudio->segment_data, (*context)->ctxaudio->segment_data_size, output_data, AUDIO_TYPE, context);
        if ((seg_gen == I2OK) || (seg_gen > I2ERROR_MAX))
            context_refresh(context, AUDIO_TYPE);
    }

    return seg_gen;
}
