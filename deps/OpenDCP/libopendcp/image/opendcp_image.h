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

#ifndef _OPENDCP_IMAGE_H_
#define _OPENDCP_IMAGE_H_

#ifdef __cplusplus
extern "C" {
#endif

enum SAMPLE_METHOD {
    SAMPLE_NONE = 0,
    NEAREST_PIXEL,
    BICUBIC
};

typedef struct {
    float r;
    float g;
    float b;
} rgb_pixel_float_t;

typedef struct {
    float x;
    float y;
    float z;
} xyz_pixel_float_t;

typedef struct {
    int component_number;
    int *data;
} odcp_image_component_t;

typedef struct {
    int x0;
    int y0;
    int x1;
    int y1;
    int dx;
    int dy;
    int w;
    int h;
    int bpp;
    int precision;
    int signed_bit;
    odcp_image_component_t *component;
    int n_components;
} odcp_image_t;

int  read_tif(odcp_image_t **image_ptr, const char *infile, int fd);
int  write_tif(odcp_image_t *image, const char *outfile, int fd);
int  read_dpx(odcp_image_t **image_ptr, int dpx, const char *infile, int fd);
odcp_image_t *odcp_image_create(int n_components, int w, int h);
void odcp_image_free(odcp_image_t *image);
int odcp_image_readline(odcp_image_t *image, int y, unsigned char *data); 
int rgb_to_xyz(odcp_image_t *image, int gamma, int method);
rgb_pixel_float_t yuv444toRGB888(int y, int cb, int cr);

#ifdef __cplusplus
}
#endif

#endif  //_OPENDCP_H_
