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

#ifndef _OPENDCP_H_
#define _OPENDCP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <opendcp_image.h>

#define MAX_ASSETS          10   /* Soft limit */
#define MAX_REELS           30   /* Soft limit */
#define MAX_PKL             1    /* Soft limit */
#define MAX_CPL             5    /* Soft limit */
#define MAX_PATH_LENGTH     4096 
#define MAX_FILENAME_LENGTH 256 
#define MAX_BASENAME_LENGTH 256 

#define MAX_DCP_JPEG_BITRATE 250000000  /* Maximum DCI compliant bit rate for JPEG2000 */
#define MAX_DCP_MPEG_BITRATE  80000000  /* Maximum DCI compliant bit rate for MPEG */

#define MAX_WIDTH_2K        2048 
#define MAX_HEIGHT_2K       1080 

static const char* OPENDCP_VERSION   = "0.0.24"; 
static const char *OPENDCP_NAME      = "OpenDCP"; 
static const char *OPENDCP_COPYRIGHT = "(c) 2010-2012 Terrence Meiczinger. All rights reserved."; 
static const char *OPENDCP_LICENSE   = "The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE."; 
static const char *OPENDCP_WEBSITE   = "http://www.opendcp.org"; 

/* XML Namespaces */
static const char *XML_HEADER = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";

static const char *NS_CPL[]   = { "None",
                                  "http://www.digicine.com/PROTO-ASDCP-CPL-20040511#",
                                  "http://www.smpte-ra.org/schemas/429-7/2006/CPL"};

static const char *NS_PKL[]   = { "None",
                                  "http://www.digicine.com/PROTO-ASDCP-PKL-20040311#",
                                  "http://www.smpte-ra.org/schemas/429-8/2007/PKL"};

static const char *NS_AM[]    = { "None",
                                  "http://www.digicine.com/PROTO-ASDCP-AM-20040311#",
                                  "http://www.smpte-ra.org/schemas/429-9/2007/AM"};

static const char *NS_CPL_3D[] = { "None",
                                   "http://www.digicine.com/schemas/437-Y/2007/Main-Stereo-Picture-CPL",
                                   "http://www.smpte-ra.org/schemas/429-10/2008/Main-Stereo-Picture-CPL"};

/* Digital signature canonicalization method algorithm */
static const char *DS_DSIG     = "http://www.w3.org/2000/09/xmldsig#"; 

/* Digital signature canonicalization method algorithm */
static const char *DS_CMA      = "http://www.w3.org/TR/2001/REC-xml-c14n-20010315";

/* Digital signature digest method algorithm */
static const char *DS_DMA      =  "http://www.w3.org/2000/09/xmldsig#sha1";

/* Digital signature signature method algorithm */
static const char *DS_SMA[]    = { "None",
                                   "http://www.w3.org/2000/09/xmldsig#rsa-sha1",
                                   "http://www.w3.org/2001/04/xmldsig-more#rsa-sha256"};

/* Digital signature transport method algorithm */
static const char *DS_TMA      = "http://www.w3.org/2000/09/xmldsig#enveloped-signature";

static const char *RATING_AGENCY[] = { "None",
                                       "http://www.mpaa.org/2003-ratings",
                                       "http://rcq.qc.ca/2003-ratings"};

static const char *DCP_LOG[] = { "NONE",
                                 "ERROR",
                                 "WARN",
                                 "INFO",
                                 "DEBUG"};

/* Defaults */
static const char *DCP_ANNOTATION = "OPENDCP-FILM";
static const char *DCP_TITLE      = "OPENDCP-FILM-TITLE";
static const char *DCP_KIND       = "feature";

enum DCP_ERROR {
    DCP_FATAL   = -1,
    DCP_SUCCESS =  0,
    DCP_WARN    =  1,
    DCP_QUIT    =  2
};

enum FILE_FILTER {
    J2K_INPUT,
    MXF_INPUT
};

enum LOG_LEVEL {
    LOG_NONE = 0,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

enum ASSET_CLASS_TYPE {
    ACT_UNKNOWN,
    ACT_PICTURE,
    ACT_SOUND,
    ACT_TIMED_TEXT
};

enum CINEMA_PROFILE {
    DCP_CINEMA2K = 3,
    DCP_CINEMA4K = 4,
};

enum ASSET_ESSENCE_TYPE {
    AET_UNKNOWN,
    AET_MPEG2_VES,
    AET_JPEG_2000,
    AET_PCM_24b_48k,
    AET_PCM_24b_96k,
    AET_TIMED_TEXT,
    AET_JPEG_2000_S
};

enum XML_NAMESPACE {
    XML_NS_UNKNOWN,
    XML_NS_INTEROP,
    XML_NS_SMPTE
};

enum J2K_ENCODER {
    J2K_OPENJPEG,
    J2K_KAKADU
};

enum DPX_MODE {
    DPX_LINEAR = 0,
    DPX_FILM,
    DPX_VIDEO
};

enum DCP_ERROR_MESSAGES {
    DCP_NO_ERROR   = 0,
    DCP_ERROR,
    DCP_FILE_OPEN_ERROR,
    DCP_NO_PICTURE_TRACK,
    DCP_MULTIPLE_PICTURE_TRACK,
    DCP_ASSET_NO_DURATION,
    DCP_INVALID_ESSENCE,
    DCP_SPECIFCATION_MISMATCH,

    /* J2K */
    J2K_ERROR,

    /* MXF */
    MXF_CALC_DIGEST_FAILED,
    MXF_COULD_NOT_DETECT_ESSENCE_TYPE,
    MXF_UNKOWN_ESSENCE_TYPE,
    MXF_MPEG2_FILE_OPEN_ERROR,
    MXF_J2K_FILE_OPEN_ERROR,
    MXF_WAV_FILE_OPEN_ERROR,
    MXF_TT_FILE_OPEN_ERROR,
    MXF_FILE_WRITE_ERROR,
    MXF_FILE_FINALIZE_ERROR,
    MXF_PARSER_RESET_ERROR,

    /* XML */

    /* COMMON */
    STRING_LENGTH_NOTEQUAL,
    STRING_NOTSEQUENTIAL,

    DCP_ERROR_MAX
};

static const char *ERR_STRING[] = {
    "NONE",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
};

typedef struct filelist_t {
    char           **in;
    char           **out;
    int            file_count;
} filelist_t;

typedef struct {
    char           filename[MAX_FILENAME_LENGTH];
} assetmap_t;

typedef struct {
    char           filename[MAX_FILENAME_LENGTH];
} volindex_t;

typedef struct {
    char           uuid[40];
    int            essence_class;
    int            essence_type;
    int            duration;
    int            intrinsic_duration;
    int            entry_point;
    int            xml_ns;
    int            stereoscopic;
    char           size[18];
    char           name[128];
    char           annotation[128];
    char           edit_rate[20];
    char           frame_rate[20];
    char           sample_rate[20];
    char           aspect_ratio[20];
    char           digest[40];
    char           filename[MAX_FILENAME_LENGTH];
} asset_t;

typedef struct {
    asset_t        asset_list[3];
    int            asset_count;
} asset_list_t;

typedef struct {
    char           uuid[40];
    char           annotation[128];
    int            asset_count; 
    asset_t        asset[MAX_ASSETS];
    asset_t        MainPicture;
    asset_t        MainSound;
    asset_t        MainSubtitle;
} reel_t;

typedef struct {
    char           uuid[40];
    char           annotation[128];
    char           size[18];
    char           digest[40];
    int            duration;
    int            entry_point;
    char           issuer[80];
    char           creator[80];
    char           title[80];
    char           timestamp[30];
    char           kind[15];
    char           rating[5];
    char           filename[MAX_FILENAME_LENGTH];
    int            reel_count;
    reel_t         reel[MAX_REELS]; 
} cpl_t;

typedef struct {
    char           uuid[40];
    char           annotation[128];
    char           size[18];
    char           issuer[80];
    char           creator[80];
    char           timestamp[30];
    char           filename[MAX_FILENAME_LENGTH];
    int            cpl_count;
    cpl_t          cpl[MAX_CPL]; 
} pkl_t;

typedef struct {
    int            start_frame;
    int            end_frame;
    int            encoder;
    int            duration;
    int            dpx;
    int            lut;
    int            xyz;
    int            xyz_method;
    int            resize;
} j2k_options_t;

typedef struct {
    int            start_frame;
    int            end_frame;
    int            duration;
} mxf_options_t;

typedef struct {
    char           basename[40];
    char           issuer[80];
    char           creator[80];
    char           timestamp[30];
    char           annotation[128];
    char           title[80];
    char           kind[15];
    char           rating[5];
    char           aspect_ratio[20];
} xml_options_t;

typedef unsigned char byte_t; 

typedef struct {
    int            cinema_profile;
    int            frame_rate;
    int            duration;
    int            entry_point;
    int            stereoscopic;
    int            slide;
    int            log_level;
    int            digest_flag;
    int            ns;
    int            encrypt_header_flag;
    int            key_flag;
    int            delete_intermediate;
    byte_t         key_id[16];
    byte_t         key_value[16];
    int            write_hmac;
    int            no_overwrite;
    int            bw;
    int            threads;
    int            xml_sign;
    int            xml_use_external_certs;
    char           *root_cert_file;
    char           *ca_cert_file;
    char           *signer_cert_file;
    char           *private_key_file; 
    char           dcp_path[MAX_BASENAME_LENGTH];
    j2k_options_t  j2k;
    mxf_options_t  mxf;
    xml_options_t  xml;
    assetmap_t     assetmap;
    volindex_t     volindex;
    int            pkl_count;
    pkl_t          pkl[MAX_PKL];
} opendcp_t;

/* common functions */
void  dcp_log(int level, const char *fmt, ...);
void  dcp_fatal(opendcp_t *opendcp, char *error);
void  get_timestamp(char *timestamp);
int   get_asset_type(asset_t asset);
int   get_file_essence_class(char *filename);
int   validate_reel(opendcp_t *opendcp, cpl_t *cpl, int reel);
int   add_reel(opendcp_t *opendcp, cpl_t *cpl, asset_list_t reel);
int   add_cpl(opendcp_t *opendcp, pkl_t *pkl);
int   add_pkl(opendcp_t *opendcp);
void  dcp_set_log_level(int log_level);
void  dcp_log_init(int level, const char *file);

/* opendcp context */
opendcp_t *create_opendcp();
int       delete_opendcp(opendcp_t *opendcp);

/* image functions */
int check_image_compliance(int profile, odcp_image_t *image, char *file);

/* ASDCPLIB functions */
int read_asset_info(asset_t *asset);
void uuid_random(char *uuid);
int calculate_digest(const char *filename, char *digest);
int get_wav_duration(const char *filename, int frame_rate);
int get_file_essence_type(char *in_path);

/* MXF functions */
int write_mxf(opendcp_t *opendcp, filelist_t *filelist, char *output);

/* XML functions */
int write_cpl(opendcp_t *opendcp, cpl_t *cpl);
int write_pkl(opendcp_t *opendcp, pkl_t *pkl);
int write_assetmap(opendcp_t *opendcp);
int write_volumeindex(opendcp_t *opendcp);
int xml_verify(char *filename);
char *base64(const unsigned char *data, int length);
char *strip_cert(const char *data);
char *strip_cert_file(char *filename);
int xml_sign(opendcp_t *opendcp, char *filename);

/* J2K functions */
int convert_to_j2k(opendcp_t *opendcp, char *in_file, char *out_file, char *tmp_path);

/* retrieve error string */
char *error_string(int error_code);

#ifdef __cplusplus
}
#endif

#endif // _OPENDCP_H_
