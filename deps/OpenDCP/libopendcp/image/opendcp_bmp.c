/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010 Terrence Meiczinger, All Rights Reserved

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

/*
 * Copyright (c) 2002-2007, Communications and Remote Sensing Laboratory, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2007, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux and Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2006-2007, Parvatha Elangovan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, bmpCLUDbmpG, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  bmp NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, bmpDIRECT, bmpCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (bmpCLUDbmpG, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSbmpESS
 * bmpTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER bmp
 * CONTRACT, STRICT LIABILITY, OR TORT (bmpCLUDbmpG NEGLIGENCE OR OTHERWISE)
 * ARISbmpG bmp ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <openjpeg.h>
#include "../opendcp.h"
#include "opendcp_image.h"
#include "opendcp_xyz.h"

/*
 * Get logarithm of an integer and round downwards.
 *
 * log2(a)
 */
static int int_floorlog2(int a) {
	int l;
	for (l = 0; a > 1; l++) {
		a >>= 1;
	}
	return l;
}

/*
 * Divide an integer by a power of 2 and round upwards.
 *
 * a divided by 2^b
 */
static int int_ceildivpow2(int a, int b) {
	return (a + (1 << b) - 1) >> b;
}

/*
 * Divide an integer and round upwards.
 *
 * a divided by b
 */
static int int_ceildiv(int a, int b) {
	return (a + b - 1) / b;
}

/* WORD defines a two byte word */
typedef unsigned short int WORD;

/* DWORD defines a four byte word */
typedef unsigned long int DWORD;

struct bmpfile_header {
  uint32_t filesz;
  uint16_t creator1;
  uint16_t creator2;
  uint32_t bmp_offset;
};

typedef struct {
  WORD bfType;			/* 'BM' for Bitmap (19776) */
  DWORD bfSize;			/* Size of the file        */
  WORD bfReserved1;		/* Reserved : 0            */
  WORD bfReserved2;		/* Reserved : 0            */
  DWORD bfOffBits;		/* Offset                  */
} BITMAPFILEHEADER_t;

typedef struct {
  DWORD biSize;			/* Size of the structure in bytes */
  DWORD biWidth;		/* Width of the image in pixels */
  DWORD biHeight;		/* Heigth of the image in pixels */
  WORD biPlanes;		/* 1 */
  WORD biBitCount;		/* Number of color bits by pixels */
  DWORD biCompression;		/* Type of encoding 0: none 1: RLE8 2: RLE4 */
  DWORD biSizeImage;		/* Size of the image in bytes */
  DWORD biXpelsPerMeter;	/* Horizontal (X) resolution in pixels/meter */
  DWORD biYpelsPerMeter;	/* Vertical (Y) resolution in pixels/meter */
  DWORD biClrUsed;		/* Number of color used in the image (0: ALL) */
  DWORD biClrImportant;		/* Number of important color (0: ALL) */
} BITMAPINFOHEADER_t;

int read_bmp(odcp_image_t **image_ptr, const char *infile, int fd) {
    FILE *bmp;
    int subsampling_dx = parameters->subsampling_dx;
    int subsampling_dy = parameters->subsampling_dy;

    int i, numcomps, w, h;
    OPJ_COLOR_SPACE color_space;
opj_image_cmptparm_t cmptparm[3];	/* maximum of 3 components */
opj_image_t * image = NULL;

	BITMAPFILEHEADER_t bmp_file_h;
	BITMAPINFOHEADER_t bmp_info_h;
	unsigned char *RGB;
	unsigned char *table_R, *table_G, *table_B;
	unsigned int j, PAD = 0;

    int index,w,h,image_size;

	int x, y;
	int gray_scale = 1, not_end_file = 1; 

	unsigned int line = 0, col = 0;
	unsigned char v, v2;
  
    /* open tiff using filename or file descriptor */
    if (fd == 0) {
        bmp = fopen(infile,"rb");
    } else {
        bmp = fd;
    }

    if (!bmp) {
        dcp_log(LOG_ERROR,"Failed to open %s for reading\n", infile);
        return DCP_FATAL;
    }

    bmp_file_h.bfType = getc(bmp);
    bmp_file_h.bfType = (getc(bmp) << 8) + bmp_file_h.bfType;
	
	if (bmp_file_h.bfType != 19778) {
		fprintf(stderr,"Error, not a BMP file!\n");
		return 0;
	} else {
		/* FILE HEADER */
		/* ------------- */
		bmp_file_h.bfSize = getc(bmp);
		bmp_file_h.bfSize = (getc(bmp) << 8) + bmp_file_h.bfSize;
		bmp_file_h.bfSize = (getc(bmp) << 16) + bmp_file_h.bfSize;
		bmp_file_h.bfSize = (getc(bmp) << 24) + bmp_file_h.bfSize;

		bmp_file_h.bfReserved1 = getc(bmp);
		bmp_file_h.bfReserved1 = (getc(bmp) << 8) + bmp_file_h.bfReserved1;

		bmp_file_h.bfReserved2 = getc(bmp);
		bmp_file_h.bfReserved2 = (getc(bmp) << 8) + bmp_file_h.bfReserved2;

		bmp_file_h.bfOffBits = getc(bmp);
		bmp_file_h.bfOffBits = (getc(bmp) << 8) + bmp_file_h.bfOffBits;
		bmp_file_h.bfOffBits = (getc(bmp) << 16) + bmp_file_h.bfOffBits;
		bmp_file_h.bfOffBits = (getc(bmp) << 24) + bmp_file_h.bfOffBits;

		/* bmpFO HEADER */
		/* ------------- */

		Info_h.biSize = getc(bmp);
		Info_h.biSize = (getc(bmp) << 8) + Info_h.biSize;
		Info_h.biSize = (getc(bmp) << 16) + Info_h.biSize;
		Info_h.biSize = (getc(bmp) << 24) + Info_h.biSize;

		Info_h.biWidth = getc(bmp);
		Info_h.biWidth = (getc(bmp) << 8) + Info_h.biWidth;
		Info_h.biWidth = (getc(bmp) << 16) + Info_h.biWidth;
		Info_h.biWidth = (getc(bmp) << 24) + Info_h.biWidth;
		w = Info_h.biWidth;

		Info_h.biHeight = getc(bmp);
		Info_h.biHeight = (getc(bmp) << 8) + Info_h.biHeight;
		Info_h.biHeight = (getc(bmp) << 16) + Info_h.biHeight;
		Info_h.biHeight = (getc(bmp) << 24) + Info_h.biHeight;
		h = Info_h.biHeight;

		Info_h.biPlanes = getc(bmp);
		Info_h.biPlanes = (getc(bmp) << 8) + Info_h.biPlanes;

		Info_h.biBitCount = getc(bmp);
		Info_h.biBitCount = (getc(bmp) << 8) + Info_h.biBitCount;

		Info_h.biCompression = getc(bmp);
		Info_h.biCompression = (getc(bmp) << 8) + Info_h.biCompression;
		Info_h.biCompression = (getc(bmp) << 16) + Info_h.biCompression;
		Info_h.biCompression = (getc(bmp) << 24) + Info_h.biCompression;

		Info_h.biSizeImage = getc(bmp);
		Info_h.biSizeImage = (getc(bmp) << 8) + Info_h.biSizeImage;
		Info_h.biSizeImage = (getc(bmp) << 16) + Info_h.biSizeImage;
		Info_h.biSizeImage = (getc(bmp) << 24) + Info_h.biSizeImage;

		Info_h.biXpelsPerMeter = getc(bmp);
		Info_h.biXpelsPerMeter = (getc(bmp) << 8) + Info_h.biXpelsPerMeter;
		Info_h.biXpelsPerMeter = (getc(bmp) << 16) + Info_h.biXpelsPerMeter;
		Info_h.biXpelsPerMeter = (getc(bmp) << 24) + Info_h.biXpelsPerMeter;

		Info_h.biYpelsPerMeter = getc(bmp);
		Info_h.biYpelsPerMeter = (getc(bmp) << 8) + Info_h.biYpelsPerMeter;
		Info_h.biYpelsPerMeter = (getc(bmp) << 16) + Info_h.biYpelsPerMeter;
		Info_h.biYpelsPerMeter = (getc(bmp) << 24) + Info_h.biYpelsPerMeter;

		Info_h.biClrUsed = getc(bmp);
		Info_h.biClrUsed = (getc(bmp) << 8) + Info_h.biClrUsed;
		Info_h.biClrUsed = (getc(bmp) << 16) + Info_h.biClrUsed;
		Info_h.biClrUsed = (getc(bmp) << 24) + Info_h.biClrUsed;

		Info_h.biClrImportant = getc(bmp);
		Info_h.biClrImportant = (getc(bmp) << 8) + Info_h.biClrImportant;
		Info_h.biClrImportant = (getc(bmp) << 16) + Info_h.biClrImportant;
		Info_h.biClrImportant = (getc(bmp) << 24) + Info_h.biClrImportant;

		/* Read the data and store them in the OUT file */
    
		if (Info_h.biBitCount == 24) {
			numcomps = 3;
			color_space = CLRSPC_SRGB;
			/* initialize image components */
			memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));
			for(i = 0; i < numcomps; i++) {
				cmptparm[i].prec = 8;
				cmptparm[i].bpp = 8;
				cmptparm[i].sgnd = 0;
				cmptparm[i].dx = subsampling_dx;
				cmptparm[i].dy = subsampling_dy;
				cmptparm[i].w = w;
				cmptparm[i].h = h;
			}
			/* create the image */
			image = opj_image_create(numcomps, &cmptparm[0], color_space);
			if(!image) {
				fclose(bmp);
				return NULL;
			}

			/* set image offset and reference grid */
			image->x0 = parameters->image_offset_x0;
			image->y0 = parameters->image_offset_y0;
			image->x1 =	!image->x0 ? (w - 1) * subsampling_dx + 1 : image->x0 + (w - 1) * subsampling_dx + 1;
			image->y1 =	!image->y0 ? (h - 1) * subsampling_dy + 1 : image->y0 + (h - 1) * subsampling_dy + 1;

			/* set image data */

			/* Place the cursor at the beginning of the image information */
			fseek(bmp, 0, SEEK_SET);
			fseek(bmp, bmp_file_h.bfOffBits, SEEK_SET);
			
			W = Info_h.biWidth;
			H = Info_h.biHeight;

			/* PAD = 4 - (3 * W) % 4; */
			/* PAD = (PAD == 4) ? 0 : PAD; */
			PAD = (3 * W) % 4 ? 4 - (3 * W) % 4 : 0;
			
			RGB = (unsigned char *) malloc((3 * W + PAD) * H * sizeof(unsigned char));
			
			fread(RGB, sizeof(unsigned char), (3 * W + PAD) * H, bmp);
			
			index = 0;

			for(y = 0; y < (int)H; y++) {
				unsigned char *scanline = RGB + (3 * W + PAD) * (H - 1 - y);
				for(x = 0; x < (int)W; x++) {
					unsigned char *pixel = &scanline[3 * x];
					image->comps[0].data[index] = pixel[2];	/* R */
					image->comps[1].data[index] = pixel[1];	/* G */
					image->comps[2].data[index] = pixel[0];	/* B */
					index++;
				}
			}

			free(RGB);

		} else if (Info_h.biBitCount == 8 && Info_h.biCompression == 0) {
			table_R = (unsigned char *) malloc(256 * sizeof(unsigned char));
			table_G = (unsigned char *) malloc(256 * sizeof(unsigned char));
			table_B = (unsigned char *) malloc(256 * sizeof(unsigned char));
			
			for (j = 0; j < Info_h.biClrUsed; j++) {
				table_B[j] = getc(bmp);
				table_G[j] = getc(bmp);
				table_R[j] = getc(bmp);
				getc(bmp);
				if (table_R[j] != table_G[j] && table_R[j] != table_B[j] && table_G[j] != table_B[j])
					gray_scale = 0;
			}
			
			/* Place the cursor at the beginning of the image information */
			fseek(bmp, 0, SEEK_SET);
			fseek(bmp, bmp_file_h.bfOffBits, SEEK_SET);
			
			W = Info_h.biWidth;
			H = Info_h.biHeight;
			if (Info_h.biWidth % 2)
				W++;
			
			numcomps = gray_scale ? 1 : 3;
			color_space = gray_scale ? CLRSPC_GRAY : CLRSPC_SRGB;
			/* initialize image components */
			memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));
			for(i = 0; i < numcomps; i++) {
				cmptparm[i].prec = 8;
				cmptparm[i].bpp = 8;
				cmptparm[i].sgnd = 0;
				cmptparm[i].dx = subsampling_dx;
				cmptparm[i].dy = subsampling_dy;
				cmptparm[i].w = w;
				cmptparm[i].h = h;
			}
			/* create the image */
			image = opj_image_create(numcomps, &cmptparm[0], color_space);
			if(!image) {
				fclose(bmp);
				return NULL;
			}

			/* set image offset and reference grid */
			image->x0 = parameters->image_offset_x0;
			image->y0 = parameters->image_offset_y0;
			image->x1 =	!image->x0 ? (w - 1) * subsampling_dx + 1 : image->x0 + (w - 1) * subsampling_dx + 1;
			image->y1 =	!image->y0 ? (h - 1) * subsampling_dy + 1 : image->y0 + (h - 1) * subsampling_dy + 1;

			/* set image data */

			RGB = (unsigned char *) malloc(W * H * sizeof(unsigned char));
			
			fread(RGB, sizeof(unsigned char), W * H, bmp);
			if (gray_scale) {
				index = 0;
				for (j = 0; j < W * H; j++) {
					if ((j % W < W - 1 && Info_h.biWidth % 2) || !(Info_h.biWidth % 2)) {
						image->comps[0].data[index] = table_R[RGB[W * H - ((j) / (W) + 1) * W + (j) % (W)]];
						index++;
					}
				}

			} else {		
				index = 0;
				for (j = 0; j < W * H; j++) {
					if ((j % W < W - 1 && Info_h.biWidth % 2) || !(Info_h.biWidth % 2)) {
						unsigned char pixel_index = RGB[W * H - ((j) / (W) + 1) * W + (j) % (W)];
						image->comps[0].data[index] = table_R[pixel_index];
						image->comps[1].data[index] = table_G[pixel_index];
						image->comps[2].data[index] = table_B[pixel_index];
						index++;
					}
				}
			}
			free(RGB);
      free(table_R);
      free(table_G);
      free(table_B);
		} else if (Info_h.biBitCount == 8 && Info_h.biCompression == 1) {				
			table_R = (unsigned char *) malloc(256 * sizeof(unsigned char));
			table_G = (unsigned char *) malloc(256 * sizeof(unsigned char));
			table_B = (unsigned char *) malloc(256 * sizeof(unsigned char));
			
			for (j = 0; j < Info_h.biClrUsed; j++) {
				table_B[j] = getc(bmp);
				table_G[j] = getc(bmp);
				table_R[j] = getc(bmp);
				getc(bmp);
				if (table_R[j] != table_G[j] && table_R[j] != table_B[j] && table_G[j] != table_B[j])
					gray_scale = 0;
			}

			numcomps = gray_scale ? 1 : 3;
			color_space = gray_scale ? CLRSPC_GRAY : CLRSPC_SRGB;
			/* initialize image components */
			memset(&cmptparm[0], 0, 3 * sizeof(opj_image_cmptparm_t));
			for(i = 0; i < numcomps; i++) {
				cmptparm[i].prec = 8;
				cmptparm[i].bpp = 8;
				cmptparm[i].sgnd = 0;
				cmptparm[i].dx = subsampling_dx;
				cmptparm[i].dy = subsampling_dy;
				cmptparm[i].w = w;
				cmptparm[i].h = h;
			}
			/* create the image */
			image = opj_image_create(numcomps, &cmptparm[0], color_space);
			if(!image) {
				fclose(bmp);
				return NULL;
			}

			/* set image offset and reference grid */
			image->x0 = parameters->image_offset_x0;
			image->y0 = parameters->image_offset_y0;
			image->x1 =	!image->x0 ? (w - 1) * subsampling_dx + 1 : image->x0 + (w - 1) * subsampling_dx + 1;
			image->y1 =	!image->y0 ? (h - 1) * subsampling_dy + 1 : image->y0 + (h - 1) * subsampling_dy + 1;

			/* set image data */
			
			/* Place the cursor at the beginning of the image information */
			fseek(bmp, 0, SEEK_SET);
			fseek(bmp, bmp_file_h.bfOffBits, SEEK_SET);
			
			RGB = (unsigned char *) malloc(Info_h.biWidth * Info_h.biHeight * sizeof(unsigned char));
            
			while (not_end_file) {
				v = getc(bmp);
				if (v) {
					v2 = getc(bmp);
					for (i = 0; i < (int) v; i++) {
						RGB[line * Info_h.biWidth + col] = v2;
						col++;
					}
				} else {
					v = getc(bmp);
					switch (v) {
						case 0:
							col = 0;
							line++;
							break;
						case 1:
							line++;
							not_end_file = 0;
							break;
						case 2:
							fprintf(stderr,"No Delta supported\n");
							opj_image_destroy(image);
							fclose(bmp);
							return NULL;
						default:
							for (i = 0; i < v; i++) {
								v2 = getc(bmp);
								RGB[line * Info_h.biWidth + col] = v2;
								col++;
							}
							if (v % 2)
								v2 = getc(bmp);
							break;
					}
				}
			}
			if (gray_scale) {
				index = 0;
				for (line = 0; line < Info_h.biHeight; line++) {
					for (col = 0; col < Info_h.biWidth; col++) {
						image->comps[0].data[index] = table_R[(int)RGB[(Info_h.biHeight - line - 1) * Info_h.biWidth + col]];
						index++;
					}
				}
			} else {
				index = 0;
				for (line = 0; line < Info_h.biHeight; line++) {
					for (col = 0; col < Info_h.biWidth; col++) {
						unsigned char pixel_index = (int)RGB[(Info_h.biHeight - line - 1) * Info_h.biWidth + col];
						image->comps[0].data[index] = table_R[pixel_index];
						image->comps[1].data[index] = table_G[pixel_index];
						image->comps[2].data[index] = table_B[pixel_index];
						index++;
					}
				}
			}
			free(RGB);
      free(table_R);
      free(table_G);
      free(table_B);
	} else {
		fprintf(stderr, 
			"Other system than 24 bits/pixels or 8 bits (no RLE coding) is not yet implemented [%d]\n", Info_h.biBitCount);
	}
	fclose(bmp);
 }
 
 return image;
}
