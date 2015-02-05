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

#ifndef __ISOFF_LIB__
#define __ISOFF_LIB__


#include "i2context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


uint32_t initVideoGenerator(byte *source_data, uint32_t size_source_data, byte *destination_data, i2ctx **context);

uint32_t initAudioGenerator(byte *source_data, uint32_t size_source_data, byte *destination_data, i2ctx **context);

uint32_t segmentGenerator(byte *source_data, uint32_t size_source_data, byte *destination_data, uint32_t media_type, i2ctx **context);

#endif
