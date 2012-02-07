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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <tiffio.h>
#include "opendcp.h"
#include "opendcp_image.h"

typedef struct {
    TIFF       *fp; 
    tstrip_t   strip;
    tdata_t    strip_data;
    tsize_t    strip_size;
    tsize_t    read_size;
    int        strip_num;
    int        image_size;
    int        w;
    int        h;
    uint16_t   bps;
    uint16_t   spp;
    uint16_t   photo;
    uint16_t   planar;
    int        supported;
} tiff_image_t;

void tif_set_strip(tiff_image_t *tif) {
    tif->strip_num  = TIFFNumberOfStrips(tif->fp);
    tif->strip_size = TIFFStripSize(tif->fp);
    tif->strip_data = _TIFFmalloc(tif->strip_size);
}

int read_tif(odcp_image_t **image_ptr, const char *infile, int fd) {
    tiff_image_t tif;
    int          i,index;
    odcp_image_t *image = 00;

    memset(&tif, 0, sizeof(tiff_image_t));

    /* open tiff using filename or file descriptor */
    dcp_log(LOG_DEBUG,"%-15.15s: opening tiff file %s","read_tif", infile);
    if (fd == 0) {
        tif.fp = TIFFOpen(infile, "r");
    } else {
        tif.fp = TIFFFdOpen(fd,infile,"r");
    }

    if (!tif.fp) {
        dcp_log(LOG_ERROR,"%-15.15s: failed to open %s for reading","read_tif", infile);
        return DCP_FATAL;
    }

    TIFFGetField(tif.fp, TIFFTAG_IMAGEWIDTH, &tif.w);
    TIFFGetField(tif.fp, TIFFTAG_IMAGELENGTH, &tif.h);
    TIFFGetField(tif.fp, TIFFTAG_BITSPERSAMPLE, &tif.bps);
    TIFFGetField(tif.fp, TIFFTAG_SAMPLESPERPIXEL, &tif.spp);
    TIFFGetField(tif.fp, TIFFTAG_PHOTOMETRIC, &tif.photo);
    TIFFGetField(tif.fp, TIFFTAG_PLANARCONFIG, &tif.planar);
    tif.image_size = tif.w * tif.h;

    dcp_log(LOG_DEBUG,"%-15.15s: tif attributes photo: %d bps: %d spp: %d planar: %d","read_tif",tif.photo,tif.bps,tif.spp,tif.planar);

    /* check if image is supported */
    switch (tif.photo) {
        case PHOTOMETRIC_YCBCR:
            if (tif.bps == 8 || tif.bps == 16 || tif.bps == 24) {
                tif.supported = 1;
            } else {
                dcp_log(LOG_ERROR,"%-15.15s: YUV/YCbCr tiff conversion failed, bitdepth %d, only 8,16,24 bits are supported","read_tif", tif.bps);
            }
            break;
        case PHOTOMETRIC_MINISWHITE:
            dcp_log(LOG_ERROR,"%-15.15s: 1-bit BW images are not supported","read_tif", tif.bps);
            break;
        case PHOTOMETRIC_MINISBLACK:
            if (tif.bps == 1 || tif.bps == 8 || tif.bps == 16 || tif.bps == 24) {
                tif.supported = 1;
            } else {
                dcp_log(LOG_ERROR,"%-15.15s: Grayscale tiff conversion failed, bitdepth %d, only 8,16,24 bits are supported","read_tif", tif.bps);
            }
            break;
        case PHOTOMETRIC_RGB:
            if (tif.bps == 8 || tif.bps == 12 || tif.bps == 16) {
                tif.supported = 1;
            } else {
                dcp_log(LOG_ERROR,"%-15.15s: RGB tiff conversion failed, bitdepth %d, only 8,12,16 bits are supported","read_tif", tif.bps);
            }
            break;
        default:
            tif.supported = 0;
            dcp_log(LOG_ERROR,"%-15.15s: tif image type not supported","read_tif");
            break;
    }

    if (tif.supported == 0) {
        TIFFClose(tif.fp);
       return(DCP_FATAL);
    }
   
    /* create the image */
    dcp_log(LOG_DEBUG,"%-15.15s: allocating odcp image","read_tif");
    image = odcp_image_create(3,tif.w,tif.h);
    
    if (!image) {
        TIFFClose(tif.fp);
        dcp_log(LOG_ERROR,"%-15.15s: failed to create image %s","read_tif",infile);
        return DCP_FATAL;
    }

    /* BW */
    if (tif.photo == PHOTOMETRIC_MINISWHITE) {
        tif_set_strip(&tif);
        uint8_t *data  = (uint8_t *)tif.strip_data;
        for (tif.strip = 0; tif.strip < tif.strip_num; tif.strip++) {
            tif.read_size = TIFFReadEncodedStrip(tif.fp, tif.strip, tif.strip_data, tif.strip_size);
            for (i=0; i<tif.image_size; i++) {
                image->component[0].data[i] = data[i] << 4; // R 
                image->component[1].data[i] = data[i] << 4; // G 
                image->component[2].data[i] = data[i] << 4; // B 
            }
        }
        _TIFFfree(tif.strip_data);
    }

    /* GRAYSCALE */
    else if (tif.photo == PHOTOMETRIC_MINISBLACK) {
        uint32_t *raster = (uint32_t*) _TIFFmalloc(tif.image_size * sizeof(uint32_t));
        TIFFReadRGBAImageOriented(tif.fp, tif.w, tif.h, raster, ORIENTATION_TOPLEFT,0);
        /* 8/16/24 bits per pixel */
        if (tif.bps==8 || tif.bps==16 || tif.bps==24) {
            for (i=0;i<tif.image_size;i++) {
                image->component[0].data[i] = (raster[i] & 0xFF)       << 4;
                image->component[1].data[i] = (raster[i] >> 8 & 0xFF)  << 4;
                image->component[2].data[i] = (raster[i] >> 16 & 0xFF) << 4;
            }
        }
        _TIFFfree(raster);
    }

    /* YUV */
    else if (tif.photo == PHOTOMETRIC_YCBCR) {
        uint32_t *raster = (uint32_t*) _TIFFmalloc(tif.image_size * sizeof(uint32_t));
        TIFFReadRGBAImageOriented(tif.fp, tif.w, tif.h, raster, ORIENTATION_TOPLEFT,0);
        /* 8/16/24 bits per pixel */
        if (tif.bps==8 || tif.bps==16 || tif.bps==24) {
            for (i=0;i<tif.image_size;i++) {
                image->component[0].data[i] = (raster[i] & 0xFF)       << 4;
                image->component[1].data[i] = (raster[i] >> 8 & 0xFF)  << 4;
                image->component[2].data[i] = (raster[i] >> 16 & 0xFF) << 4;
            }
        }
        _TIFFfree(raster);
    }

    /* RGB(A) and GRAYSCALE */
    else if (tif.photo == PHOTOMETRIC_RGB) {
        tif_set_strip(&tif);
        uint8_t *data  = (uint8_t *)tif.strip_data;

        /* 8 bits per pixel */
        if (tif.bps==8) {
            index = 0;
            for (tif.strip = 0; tif.strip < tif.strip_num; tif.strip++) {
                tif.read_size = TIFFReadEncodedStrip(tif.fp, tif.strip, tif.strip_data, tif.strip_size);
                for (i=0; i<tif.read_size && index<tif.image_size; i+=tif.spp) {
                    /* rounded to 12 bits */
                    image->component[0].data[index] = data[i+0] << 4; // R 
                    image->component[1].data[index] = data[i+1] << 4; // G 
                    image->component[2].data[index] = data[i+2] << 4; // B 
                    index++;
                }
            }
        /*12 bits per pixel*/
        } else if (tif.bps==12) {
            index = 0;
            for (tif.strip = 0; tif.strip < tif.strip_num; tif.strip++) {
                tif.read_size = TIFFReadEncodedStrip(tif.fp, tif.strip, tif.strip_data, tif.strip_size);
                for (i=0; i<tif.read_size && (index+1)<tif.image_size; i+=(3*tif.spp)) {
                    image->component[0].data[index]   = ( data[i+0] << 4)         | (data[i+1] >> 4); // R
                    image->component[1].data[index]   = ((data[i+1] & 0x0f) << 8) | (data[i+2]);      // G
                    image->component[2].data[index]   = ( data[i+3] << 4)         | (data[i+4] >> 4); // B
                    if (tif.spp == 4) {
                        /* skip alpha channel */
                        image->component[0].data[index+1] = ( data[i+6] << 4)         | (data[i+7] >> 4);  // R
                        image->component[1].data[index+1] = ((data[i+7] & 0x0f) << 8) | (data[i+8]);       // G
                        image->component[2].data[index+1] = ( data[i+9] << 4)         | (data[i+10] >> 4); // B
                    } else {
                        image->component[0].data[index+1] = ((data[i+4] & 0x0f) << 8) | (data[i+5]);       // R
                        image->component[1].data[index+1] = ( data[i+6] <<4 )         | (data[i+7] >> 4);  // G
                        image->component[2].data[index+1] = ((data[i+7] & 0x0f) << 8) | (data[i+8]);       // B
                    }
                    index+=2;
                }
            }
        /* 16 bits per pixel */
        } else if (tif.bps==16) {
            index = 0;
            for (tif.strip = 0; tif.strip < tif.strip_num; tif.strip++) {
                tif.read_size = TIFFReadEncodedStrip(tif.fp, tif.strip, tif.strip_data, tif.strip_size);
                for (i=0; i<tif.read_size && index<tif.image_size; i+=(2*tif.spp)) {
                    /* rounded to 12 bits */
                    image->component[0].data[index] = ((data[i+1] << 8) | data[i+0]) >> 4; // R 
                    image->component[1].data[index] = ((data[i+3] << 8) | data[i+2]) >> 4; // G 
                    image->component[2].data[index] = ((data[i+5] << 8) | data[i+4]) >> 4; // B 
                    index++;
                }
            }
        }
        _TIFFfree(tif.strip_data);
    }

    TIFFClose(tif.fp);

    dcp_log(LOG_DEBUG,"%-15.15s: tiff read complete","read_tif");
    *image_ptr = image;

    return DCP_SUCCESS;
}

int write_tif(odcp_image_t *image, const char *outfile, int fd) {
    int y;
    TIFF *tif;
    tdata_t data;

    /* open tiff using filename or file descriptor */
    if (fd == 0) {
        tif = TIFFOpen(outfile, "wb");
    } else {
        tif = TIFFFdOpen(fd, outfile, "wb");
    }

    if (tif == NULL) {
        dcp_log(LOG_ERROR, "%-15.15s: failed to open file %s for writing","write_tif", outfile);
        return DCP_FATAL;
    }

    /* Set tags */
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, image->w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, image->h);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, image->precision);
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);

    /* allocate memory for read line */
    data = _TIFFmalloc(TIFFScanlineSize(tif));

    if (data == NULL) {
        dcp_log(LOG_ERROR, "%-15.15s: tiff memory allocation error: %s", "write_tif", outfile);
        return DCP_FATAL;
    }

    /* write each row */
    for (y = 0; y<image->h; y++) {
        odcp_image_readline(image, y, data);
        TIFFWriteScanline(tif, data, y, 0);
    }

    _TIFFfree(data);
    TIFFClose(tif);

    return DCP_SUCCESS;
}
