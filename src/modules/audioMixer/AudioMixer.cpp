/*
 *  AudioMixer - Audio mixer structure
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */

#define BPS 2
 
#include "AudioMixer.hh"
#include "../../AudioFrame.hh"
#include <iostream>

bool AudioMixer::mix(Frame* input1, Frame* input2, Frame* output)
{
    AudioFrame* aInput1 = dynamic_cast<AudioFrame*>(input1);
    AudioFrame* aOutput = dynamic_cast<AudioFrame*>(output);

    int samples = aInput1->getSamples();

    for (int c=0; c<aInput1->getChannels(); c++) {
        mixChannel(aInput1->getPlanarDataBuf()[c], input2->getPlanarDataBuf()[c], aOutput->getPlanarDataBuf()[c], aInput1->getSamples());
    }

    aOutput->setSamples(aInput1->getSamples());
    aOutput->setLength(aInput1->getLength());
}

bool AudioMixer::mixChannel(unsigned char* b1, unsigned char* b2, unsigned char* bo, unsigned int samples)
{
    short b1Value = 0;
    float fB1Value = 0;
    short b2Value = 0;
    float fB2Value = 0;
    short boValue = 0;
    float fBoValue = 0;
    
    for (unsigned int i=0; i<samples*BPS; i+=BPS) {
        b1Value = (short)(b1[i] | b1[i+1] << 8);
        b2Value = (short)(b2[i] | b2[i+1] << 8);

        fB1Value = b1Value / (32768.0f);
        fB2Value = b2Value / (32768.0f);

    //    boValue = b1Value + b2Value - (b1Value * b2Value)/65536;
        fBoValue = fB1Value*0.5 + fB2Value*0.5;

        boValue = fBoValue * 32768.0;

        //printf("%d\t %f\t %f\t %d\n\n", b1Value, fB1Value, fBoValue, boValue);

      //  printf("fBoValue: %f\n", fBoValue);
      //  printf("fboValue*0.50: %f\n\n", fBoValue*0.5);

        bo[i] = boValue & 0xFF; 
        bo[i+1] = (boValue >> 8) & 0xFF;
    //    bo[i] = b1[i];
    //    bo[i+1] = b1[i+1];

       // printf("\nbo: %d, %x %x", boValue, bo[i], bo[i+1]);
       //printf(" b1: %d", b1Value);
       // printf("\nb2: %d", b2Value);
       // printf("\nInp: %x %x %x\n\n", b1[i], b1[i+1], b1[i+2]);
    }
    //printf("\n\n");
}



