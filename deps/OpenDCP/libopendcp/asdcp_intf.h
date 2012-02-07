/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2011 Terrence Meiczinger, All Rights Reserved

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <KM_error.h>
#include "opendcp.h" 

extern "C" void uuid_random(char *uuid);
extern "C" int calculate_digest(const char *filename, char *digest);
extern "C" int get_wav_duration(const char *filename, int frame_rate);

// internal functions
int write_j2k_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file);
int write_j2k_s_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file);
int write_pcm_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file);
int write_tt_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file);
int write_mpeg2_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output_file);
