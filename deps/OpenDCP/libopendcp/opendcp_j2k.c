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
#include <libgen.h>
#include <sys/stat.h>
#include <openjpeg.h>
#include <stdbool.h>
#ifdef OPENMP
#include <omp.h>
#endif
#include "opendcp.h"

#ifndef WIN32
#define strnicmp strncasecmp
#endif

int encode_kakadu(opendcp_t *opendcp, char *in_file, char *out_file);
int encode_openjpeg(opendcp_t *opendcp, opj_image_t *opj_image, char *out_file); 

static int initialize_4K_poc(opj_poc_t *POC, int numres){
    POC[0].tile  = 1; 
    POC[0].resno0  = 0; 
    POC[0].compno0 = 0;
    POC[0].layno1  = 1;
    POC[0].resno1  = numres-1;
    POC[0].compno1 = 3;
    POC[0].prg1 = CPRL;
    POC[1].tile  = 1;
    POC[1].resno0  = numres-1; 
    POC[1].compno0 = 0;
    POC[1].layno1  = 1;
    POC[1].resno1  = numres;
    POC[1].compno1 = 3;
    POC[1].prg1 = CPRL;
    return 2;
}

void set_cinema_encoder_parameters(opendcp_t *opendcp, opj_cparameters_t *parameters){
    parameters->tile_size_on = false;
    parameters->cp_tdx=1;
    parameters->cp_tdy=1;

    /* Tile part */
    parameters->tp_flag = 'C';
    parameters->tp_on = 1;

    /* Tile and Image shall be at (0,0) */
    parameters->cp_tx0 = 0;
    parameters->cp_ty0 = 0;
    parameters->image_offset_x0 = 0;
    parameters->image_offset_y0 = 0;

    /*Codeblock size= 32*32*/
    parameters->cblockw_init = 32;
    parameters->cblockh_init = 32;
    parameters->csty |= 0x01;

    /* The progression order shall be CPRL */
    parameters->prog_order = CPRL;

    /* No ROI */
    parameters->roi_compno = -1;

    parameters->subsampling_dx = 1;
    parameters->subsampling_dy = 1;

    /* 9-7 transform */
    parameters->irreversible = 1;

    parameters->tcp_rates[0] = 0;
    parameters->tcp_numlayers++;
    parameters->cp_disto_alloc = 1;

    parameters->cp_rsiz = opendcp->cinema_profile;
    if ( opendcp->cinema_profile == DCP_CINEMA4K ) {
            parameters->numpocs = initialize_4K_poc(parameters->POC,parameters->numresolution);
    }
}

int convert_to_j2k(opendcp_t *opendcp, char *in_file, char *out_file, char *tmp_path) {
    odcp_image_t *odcp_image;
    int result = 0;

    if (tmp_path == NULL) {
        tmp_path = "./";
    }
    dcp_log(LOG_DEBUG,"%-15.15s: reading input file %s","convert_to_j2k",in_file);
     
    #ifdef OPENMP
    #pragma omp critical
    #endif
    {
    result = read_image(&odcp_image,in_file); 
    }

    if (result != DCP_SUCCESS) {
        dcp_log(LOG_ERROR,"Unable to read tiff file %s",in_file);
        return DCP_FATAL;
    }

    if (!odcp_image) {
        dcp_log(LOG_ERROR,"Unable to load tiff file %s",in_file);
        return DCP_FATAL;
    }

    /* verify image is dci compliant */
    if (check_image_compliance(opendcp->cinema_profile, odcp_image, NULL) != DCP_SUCCESS) {
        dcp_log(LOG_WARN,"The image resolution of %s is not DCI Compliant",in_file);

        /* resize image */
        if (opendcp->j2k.resize) {
            if (resize(&odcp_image, opendcp->cinema_profile, opendcp->j2k.resize) != DCP_SUCCESS) {
                odcp_image_free(odcp_image);
                return DCP_FATAL;
            }
        } else {
            odcp_image_free(odcp_image);
            return DCP_FATAL;
        }
    }
    
    if (opendcp->j2k.xyz) {
        dcp_log(LOG_INFO,"RGB->XYZ color conversion %s",in_file);
        if (rgb_to_xyz(odcp_image,opendcp->j2k.lut,opendcp->j2k.xyz_method)) {
            dcp_log(LOG_ERROR,"Color conversion failed %s",in_file);
            odcp_image_free(odcp_image);
            return DCP_FATAL;
        }
    }

    if ( opendcp->j2k.encoder == J2K_KAKADU ) {
        char tempfile[255];
        sprintf(tempfile,"%s/tmp_%s.tif",tmp_path,basename(in_file));
        dcp_log(LOG_DEBUG,"%-15.15s: Writing temporary tif %s","convet_to_j2k",tempfile);
        result = write_tif(odcp_image,tempfile,0);
        odcp_image_free(odcp_image);
        
        if (result != DCP_SUCCESS) {
            dcp_log(LOG_ERROR,"Writing temporary tif failed");
            return DCP_FATAL;
        }

        result = encode_kakadu(opendcp, tempfile, out_file);
        if ( result != DCP_SUCCESS) {
            dcp_log(LOG_ERROR,"Kakadu JPEG2000 conversion failed %s",in_file);
            remove(tempfile);
            return DCP_FATAL;
        }
        remove(tempfile);
    } else {
        opj_image_t *opj_image;
        odcp_to_opj(odcp_image, &opj_image); 
        odcp_image_free(odcp_image);
        if (encode_openjpeg(opendcp,opj_image,out_file) != DCP_SUCCESS) {
            dcp_log(LOG_ERROR,"OpenJPEG JPEG2000 conversion failed %s",in_file);
            opj_image_destroy(opj_image);
            return DCP_FATAL;
        }        
        opj_image_destroy(opj_image);
    }

    return DCP_SUCCESS;
}

int encode_kakadu(opendcp_t *opendcp, char *in_file, char *out_file) {
    int j,result;
    int max_cs_len;
    int max_comp_size;
    char k_lengths[128];
    char cmd[512];
    FILE *cmdfp = NULL;
    int bw;

    if (opendcp->bw) {
        bw = opendcp->bw;
    } else {
        bw = MAX_DCP_JPEG_BITRATE;
    }

    /* set the max image and component sizes based on frame_rate */
    max_cs_len = ((float)bw)/8/opendcp->frame_rate;
    
    /* adjust cs for 3D */
    if (opendcp->stereoscopic) {
        max_cs_len = max_cs_len/2;
    } 

    max_comp_size = ((float)max_cs_len)/1.25;

    sprintf(k_lengths,"Creslengths=%d",max_cs_len);
    for (j=0;j<3;j++) {
        sprintf(k_lengths,"%s Creslengths:C%d=%d,%d",k_lengths,j,max_cs_len,max_comp_size);
    }

    if (opendcp->cinema_profile == DCP_CINEMA2K) {
        sprintf(cmd,"kdu_compress -i \"%s\" -o \"%s\" Sprofile=CINEMA2K %s -quiet",in_file,out_file,k_lengths);
    } else { 
        sprintf(cmd,"kdu_compress -i \"%s\" -o \"%s\" Sprofile=CINEMA4K %s -quiet",in_file,out_file,k_lengths);
    }
    cmdfp=popen(cmd,"r");
    result=pclose(cmdfp);
    
    if (result) {
            return DCP_FATAL;
    }

    return DCP_SUCCESS;
}

int encode_openjpeg(opendcp_t *opendcp, opj_image_t *opj_image, char *out_file) {
    bool result;
    int codestream_length;
    int max_comp_size;
    int max_cs_len;
    opj_cparameters_t parameters;
    opj_cio_t *cio = NULL;
    opj_cinfo_t *cinfo = NULL;
    FILE *f = NULL; 
    int bw;
   
    if (opendcp->bw) {
        bw = opendcp->bw;
    } else {
        bw = MAX_DCP_JPEG_BITRATE;
    }

    /* set the max image and component sizes based on frame_rate */
    max_cs_len = ((float)bw)/8/opendcp->frame_rate;
 
    /* adjust cs for 3D */
    if (opendcp->stereoscopic) {
        max_cs_len = max_cs_len/2;
    } 
 
    max_comp_size = ((float)max_cs_len)/1.25;

    /* set encoding parameters to default values */
    opj_set_default_encoder_parameters(&parameters);

    /* set default cinema parameters */
    set_cinema_encoder_parameters(opendcp, &parameters);

    parameters.cp_comment = (char*)malloc(strlen(OPENDCP_NAME)+1);
    sprintf(parameters.cp_comment,"%s", OPENDCP_NAME);

    /* adjust cinema enum type */
    if (opendcp->cinema_profile == DCP_CINEMA4K) {
        parameters.cp_cinema = CINEMA4K_24;
    } else {
        parameters.cp_cinema = CINEMA2K_24;
    }

    /* Decide if MCT should be used */
    parameters.tcp_mct = opj_image->numcomps == 3 ? 1 : 0;

    /* set max image */
    parameters.max_comp_size = max_comp_size;
    parameters.tcp_rates[0]= ((float) (opj_image->numcomps * opj_image->comps[0].w * opj_image->comps[0].h * opj_image->comps[0].prec))/
                              (max_cs_len * 8 * opj_image->comps[0].dx * opj_image->comps[0].dy);

    /* get a J2K compressor handle */
    dcp_log(LOG_DEBUG,"%-15.15s: creating compressor %s","encode_openjpeg",out_file);
    cinfo = opj_create_compress(CODEC_J2K);

    /* set event manager to null (openjpeg 1.3 bug) */
    cinfo->event_mgr = NULL;

    /* setup the encoder parameters using the current image and user parameters */
    dcp_log(LOG_DEBUG,"%-15.15s: setup J2k encoder %s","encode_openjpeg",out_file);
    opj_setup_encoder(cinfo, &parameters, opj_image);

    /* open a byte stream for writing */
    /* allocate memory for all tiles */
    dcp_log(LOG_DEBUG,"%-15.15s: opening J2k output stream %s","encode_openjpeg",out_file);
    cio = opj_cio_open((opj_common_ptr)cinfo, NULL, 0);

    dcp_log(LOG_INFO,"Encoding file %s",out_file);
    result = opj_encode(cinfo, cio, opj_image, NULL);
    dcp_log(LOG_DEBUG,"%-15.15s: encoding file %s complete","encode_openjepg",out_file);

    if (!result) {
        dcp_log(LOG_ERROR,"Unable to encode jpeg2000 file %s",out_file);
        opj_cio_close(cio);
        opj_destroy_compress(cinfo);
        return DCP_FATAL;
    }
      
    codestream_length = cio_tell(cio);

    f = fopen(out_file, "wb");

    if (!f) {
        dcp_log(LOG_ERROR,"Unable to write jpeg2000 file %s",out_file);
        opj_cio_close(cio);
        opj_destroy_compress(cinfo);
        return DCP_FATAL;
    }

    fwrite(cio->buffer, 1, codestream_length, f);
    fclose(f);

    /* free openjpeg structure */
    opj_cio_close(cio);
    opj_destroy_compress(cinfo);

    /* free user parameters structure */
    if(parameters.cp_comment) free(parameters.cp_comment);
    if(parameters.cp_matrice) free(parameters.cp_matrice);

    return DCP_SUCCESS;
}
