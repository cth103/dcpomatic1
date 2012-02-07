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

#include <string.h>
#ifdef WIN32
#include "win32/opendcp_win32_getopt.h"
#else
#include <getopt.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <opendcp.h>
#include "opendcp_cli.h"

#ifndef WIN32
#define strnicmp strncasecmp
#endif

int get_filelist(opendcp_t *opendcp,char *in_path,filelist_t *filelist);

void version() {
    FILE *fp;

    fp = stdout;
    fprintf(fp,"\n%s version %s %s\n\n",OPENDCP_NAME,OPENDCP_VERSION,OPENDCP_COPYRIGHT);

    exit(0);
}

void dcp_usage() {
    FILE *fp;
    fp = stdout;

    fprintf(fp,"\n%s version %s %s\n\n",OPENDCP_NAME,OPENDCP_VERSION,OPENDCP_COPYRIGHT);
    fprintf(fp,"Usage:\n");
    fprintf(fp,"       opendcp_mxf -i <file> -o <file> [options ...]\n\n");
    fprintf(fp,"Required:\n");
    fprintf(fp,"       -i | --input <file | dir>      - input file or directory.\n");
    fprintf(fp,"       -1 | --left <dir>              - left channel input images when creating a 3D essence\n");
    fprintf(fp,"       -2 | --right <dir>             - right channel input images when creating a 3D essence\n");
    fprintf(fp,"       -o | --output <file>           - output mxf file\n");
    fprintf(fp,"\n");
    fprintf(fp,"Options:\n");
    fprintf(fp,"       -n | --ns <interop | smpte>    - Generate SMPTE or MXF Interop labels (default smpte)\n");
    fprintf(fp,"       -r | --rate <rate>             - frame rate (default 24)\n");
    fprintf(fp,"       -s | --start <frame>           - start frame\n");
    fprintf(fp,"       -d | --end  <frame>            - end frame\n");
    fprintf(fp,"       -l | --log_level <level>       - Sets the log level 0:Quiet, 1:Error, 2:Warn (default),  3:Info, 4:Debug\n");
    fprintf(fp,"       -h | --help                    - show help\n");
    fprintf(fp,"       -v | --version                 - show version\n");
    fprintf(fp,"\n\n");
    
    fclose(fp);
    exit(0);
}

int get_filelist_3d(opendcp_t *opendcp,char *in_path_left,char *in_path_right,filelist_t *filelist) {
    filelist_t  *filelist_left;
    filelist_t  *filelist_right;
    int x,y;

    filelist_left  = filelist_alloc(filelist->file_count/2);
    filelist_right = filelist_alloc(filelist->file_count/2);

    get_filelist(opendcp,in_path_left,filelist_left);
    get_filelist(opendcp,in_path_right,filelist_right);

    if (filelist_left->file_count != filelist_right->file_count) {
        dcp_log(LOG_ERROR,"Mismatching file count for 3D images left: %d right: %d\n",filelist_left->file_count,filelist_right->file_count);
        filelist_free(filelist_left);
        filelist_free(filelist_right);
        return DCP_FATAL; 
    }

    y = 0;
    filelist->file_count = filelist_left->file_count * 2;
    for (x=0;x<filelist_left->file_count;x++) {
        strcpy(filelist->in[y++],filelist_left->in[x]);
        strcpy(filelist->in[y++],filelist_right->in[x]);
    }

    filelist_free(filelist_left);
    filelist_free(filelist_right);

    return DCP_SUCCESS;
}

int get_filelist(opendcp_t *opendcp,char *in_path,filelist_t *filelist) {
    struct stat st_in;

    if (stat(in_path, &st_in) != 0 ) {
        dcp_log(LOG_ERROR,"Could not open input file %s",in_path);
        return DCP_FATAL;
    }

    if (S_ISDIR(st_in.st_mode)) {
        build_filelist(in_path, NULL, filelist, MXF_INPUT);
    } else {
        /* mpeg2 or time_text */
        int essence_type = get_file_essence_type(in_path);
        if (essence_type == AET_UNKNOWN) {
            return DCP_FATAL;
        }
        filelist->file_count = 1;
        sprintf(filelist->in[0],"%s",in_path);
    }

    return DCP_SUCCESS;
}

int main (int argc, char **argv) {
    int c,count;
    opendcp_t *opendcp;
    char *in_path = NULL;
    char *in_path_left = NULL;
    char *in_path_right = NULL;
    char *out_path = NULL;
    filelist_t *filelist;

    if ( argc <= 1 ) {
        dcp_usage();
    }

    opendcp = create_opendcp();

    /* set initial values */
    opendcp->log_level = LOG_WARN;
    opendcp->ns = XML_NS_SMPTE;
    opendcp->frame_rate = 24;
    opendcp->threads = 4;

    /* parse options */
    while (1)
    {
        static struct option long_options[] =
        {
            {"help",           required_argument, 0, 'h'},
            {"input",          required_argument, 0, 'i'},
            {"left",           required_argument, 0, '1'},
            {"right",          required_argument, 0, '2'},
            {"ns",             required_argument, 0, 'n'},
            {"output",         required_argument, 0, 'o'},
            {"start",          required_argument, 0, 's'},
            {"end",            required_argument, 0, 'd'},
            {"rate",           required_argument, 0, 'r'},
            {"profile",        required_argument, 0, 'p'},
            {"log_level",      required_argument, 0, 'l'},
            {"version",        no_argument,       0, 'v'},
            {0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;
     
        c = getopt_long (argc, argv, "1:2:d:i:n:o:r:s:p:l:3hv",
                         long_options, &option_index);
     
        /* Detect the end of the options. */
        if (c == -1)
            break;
     
        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                   break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                   printf (" with arg %s", optarg);
                 printf ("\n");
            break;

            case '3':
               opendcp->stereoscopic = 1;
            break;

            case 'd':
               opendcp->mxf.end_frame = atoi(optarg);
               if (opendcp->mxf.end_frame < 1) {
                   dcp_fatal(opendcp,"End frame  must be greater than 0");
               }
            break;

            case 's':
               opendcp->mxf.start_frame = atoi(optarg);
               if (opendcp->mxf.start_frame < 1) {
                   dcp_fatal(opendcp,"Start frame must be greater than 0");
               }
            break;

            case 'n':
               if (!strcmp(optarg,"smpte")) {
                   opendcp->ns = XML_NS_SMPTE;
               } else if (!strcmp(optarg,"interop")) {
                   opendcp->ns = XML_NS_INTEROP;
               } else {
                   dcp_fatal(opendcp,"Invalid profile argument, must be smpte or interop");
               }
            break;

            case 'i':
               in_path = optarg;
            break;

            case '1':
               in_path_left = optarg;
               opendcp->stereoscopic = 1;
            break;

            case '2':
               in_path_right = optarg;
               opendcp->stereoscopic = 1;
            break;

            case 'l':
               opendcp->log_level = atoi(optarg);
            break;

            case 'o':
               out_path = optarg;
            break;

            case 'h':
               dcp_usage();
            break;

            case 'r':
               opendcp->frame_rate = atoi(optarg);
               if (opendcp->frame_rate > 60 || opendcp->frame_rate < 1 ) {
                   dcp_fatal(opendcp,"Invalid frame rate. Must be between 1 and 60.");
               }
            break;

            case 'v':
               version();
            break;
        }
    }

    /* set log level */
    dcp_set_log_level(opendcp->log_level);

    if (opendcp->log_level > 0) {
        printf("\nOpenDCP MXF %s %s\n",OPENDCP_VERSION,OPENDCP_COPYRIGHT);
    }

    if (opendcp->stereoscopic) {
        if (in_path_left == NULL) {
            dcp_fatal(opendcp,"3D input detected, but missing left image input path");
        } else if (in_path_right == NULL) {
            dcp_fatal(opendcp,"3D input detected, but missing right image input path");
        }
    } else {
        if (in_path == NULL) {
            dcp_fatal(opendcp,"Missing input file");
        }
    }

    if (out_path == NULL) {
        dcp_fatal(opendcp,"Missing output file");
    }

    if (opendcp->stereoscopic) {
        count = get_file_count(in_path_left, MXF_INPUT);
        filelist = (filelist_t *)filelist_alloc(count*2);
        get_filelist_3d(opendcp,in_path_left,in_path_right,filelist);
    } else {
        count = get_file_count(in_path, MXF_INPUT);
        filelist = (filelist_t *)filelist_alloc(count);
        get_filelist(opendcp,in_path,filelist);
    }

    if (filelist->file_count < 1) {
        dcp_fatal(opendcp,"No input files located");
    }
  
    if (opendcp->mxf.end_frame) {
        if (opendcp->mxf.end_frame > filelist->file_count) {
            dcp_fatal(opendcp,"End frame is greater than the actual frame count");
        }
    } else {
        opendcp->mxf.end_frame = filelist->file_count;
    }

    if (opendcp->mxf.start_frame) {
        if (opendcp->mxf.start_frame > opendcp->mxf.end_frame) {
            dcp_fatal(opendcp,"Start frame must be less than end frame");
        }
    } else {
        opendcp->mxf.start_frame = 1;
    }

    opendcp->mxf.duration = opendcp->mxf.end_frame - (opendcp->mxf.start_frame-1);  

    if (opendcp->mxf.duration < 1) {
        dcp_fatal(opendcp,"Duration must be at least 1 frame");
    }

    if (write_mxf(opendcp,filelist,out_path) != 0 )  {
        printf("Error!\n");
    }

    filelist_free(filelist);

    dcp_log(LOG_INFO,"MXF creation complete");

    if (opendcp->log_level > 0) {
        printf("\n");
    }

    delete_opendcp(opendcp);

    exit(0);
}
