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

#include "opendcp.h"

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
    fprintf(fp,"       opendcp_xml --reel <mxf mxf mxf> [options]\n\n");
    fprintf(fp,"Ex:\n");
    fprintf(fp,"       opendcp_xml --reel picture.mxf sound.mxf\n");
    fprintf(fp,"       opendcp_xml --reel picture.mxf sound.mxf --reel picture.mxf sound.mxf\n");
    fprintf(fp,"       opendcp_xml --reel picture.mxf sound.mxf subtitle.mxf --reel picture.mxf sound.mxf\n");
    fprintf(fp,"       opendcp_xml --reel picture.mxf sound.mxf --digest OpenDCP --kind trailer\n");
    fprintf(fp,"\n");
    fprintf(fp,"Required: At least 1 reel is required:\n");
    fprintf(fp,"       -r | --reel <mxf mxf mxf>      - Creates a reel of MXF elements. The first --reel is reel 1, second --reel is reel 2, etc.\n");
    fprintf(fp,"                                        The argument is a space separated list of the essence elemements.\n");
    fprintf(fp,"                                        Picture/Sound/Subtitle (order of the mxf files in the list doesn't matter)\n");
    fprintf(fp,"                                        *** a picture mxf is required per reel ***\n");
    fprintf(fp,"Options:\n");
    fprintf(fp,"       -h | --help                    - Show help\n");
    fprintf(fp,"       -v | --version                 - Show version\n");
    fprintf(fp,"       -d | --digest                  - Generates digest (used to validate DCP asset integrity)\n");
#ifdef XMLSEC
    fprintf(fp,"       -s | --sign                    - Writes XML digital signature\n");
    fprintf(fp,"       -1 | --root                    - Root pem certificate used to sign XML files\n");
    fprintf(fp,"       -2 | --ca                      - CA (intermediate) pem certificate used to sign XML files\n");
    fprintf(fp,"       -3 | --signer                  - Signer (leaf) pem certificate used to sign XML files\n");
    fprintf(fp,"       -p | --privatekey              - Private (signer) pem key used to sign XML files\n");
#endif
    fprintf(fp,"       -i | --issuer <issuer>         - Issuer details\n");
    fprintf(fp,"       -a | --annotation <annotation> - Asset annotations\n");
    fprintf(fp,"       -t | --title <title>           - DCP content title\n");
    fprintf(fp,"       -b | --base <basename>         - Prepend CPL/PKL filenames with basename rather than UUID\n");
    fprintf(fp,"       -n | --duration <duration>     - Set asset durations in frames\n");
    fprintf(fp,"       -m | --rating <rating>         - Set DCP MPAA rating G PG PG-13 R NC-17 (default none)\n");
    fprintf(fp,"       -e | --entry <entry point>     - Set asset entry point (offset) frame\n");
    fprintf(fp,"       -k | --kind <kind>             - Content kind (test, feature, trailer, policy, teaser, etc)\n");
    fprintf(fp,"       -x | --width                   - Force aspect width (overrides detect value)\n");
    fprintf(fp,"       -y | --height                  - Force aspect height (overrides detected value)\n");
    fprintf(fp,"       -l | --log_level <level>       - Set the log level 0:Quiet, 1:Error, 2:Warn (default),  3:Info, 4:Debug\n");
    fprintf(fp,"\n\n");

    fclose(fp);
    exit(0);
}

int main (int argc, char **argv) {
    int c,j;
    int reel_count=0;
    int height = 0;
    int width  = 0;
    char buffer[80];
    opendcp_t *opendcp;
    asset_list_t reel_list[MAX_REELS];

    if ( argc <= 1 ) {
        dcp_usage();
    }

    opendcp = create_opendcp();

    /* parse options */
    while (1)
    {
        static struct option long_options[] =
        {
            {"annotation",     required_argument, 0, 'a'},
            {"base",           required_argument, 0, 'b'},
            {"digest",         no_argument,       0, 'd'},
            {"duration",       required_argument, 0, 'n'},
            {"entry",          required_argument, 0, 'e'},
            {"help",           no_argument,       0, 'h'},
            {"issuer",         required_argument, 0, 'i'},
            {"kind",           required_argument, 0, 'k'},
            {"log_level",      required_argument, 0, 'l'},
            {"rating",         required_argument, 0, 'm'},
            {"reel",           required_argument, 0, 'r'},
            {"title",          required_argument, 0, 't'},
            {"root",           required_argument, 0, '1'},
            {"ca",             required_argument, 0, '2'},
            {"signer",         required_argument, 0, '3'},
            {"privatekey",     required_argument, 0, 'p'},
            {"sign",           no_argument,       0, 's'},
            {"height",         required_argument, 0, 'y'},
            {"width",          required_argument, 0, 'x'},
            {"version",        no_argument,       0, 'v'},
            {0, 0, 0, 0}
        };

        /* getopt_long stores the option index here. */
        int option_index = 0;
     
        c = getopt_long (argc, argv, "a:b:e:svdhi:k:r:l:m:n:t:x:y:p:1:2:3:",
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
            break;

            case 'a':
               sprintf(opendcp->xml.annotation,"%.128s",optarg);
            break;

            case 'b':
               sprintf(opendcp->xml.basename,"%.80s",optarg);
            break;

            case 'd':
               opendcp->digest_flag = 1;
            break;

            case 'e':
               opendcp->entry_point = atoi(optarg);
            break;

            case 'h':
               dcp_usage();
            break;
     
            case 'i':
               sprintf(opendcp->xml.issuer,"%.80s",optarg);
            break;

            case 'k':
               sprintf(opendcp->xml.kind,"%.15s",optarg);
            break;

            case 'l':
               opendcp->log_level = atoi(optarg);
            break;

            case 'm':
               if ( !strcmp(optarg,"G")
                    || !strcmp(optarg,"PG")
                    || !strcmp(optarg,"PG-13")
                    || !strcmp(optarg,"R")
                    || !strcmp(optarg,"NC-17") ) {
                   sprintf(opendcp->xml.rating,"%.5s",optarg);
               } else {
                   sprintf(buffer,"Invalid rating %s\n",optarg);
                   dcp_fatal(opendcp,buffer);
               }
            break;

            case 'n':
               opendcp->duration = atoi(optarg);
            break;
     
            case 'r':
               j = 0;
               optind--;
               while ( optind<argc && strncmp("-",argv[optind],1) != 0) {
				   sprintf(reel_list[reel_count].asset_list[j++].filename,"%s",argv[optind++]);
               }
               reel_list[reel_count++].asset_count = j--;
            break;

#ifdef XMLSEC
            case 's':
                opendcp->xml_sign = 1;

            break;
#endif

            case 't':
               sprintf(opendcp->xml.title,"%.80s",optarg);
            break;

            case 'x':
               width = atoi(optarg);
            break;

            case 'y':
               height = atoi(optarg);
            break;

            case '1':
               opendcp->root_cert_file = optarg;
               opendcp->xml_use_external_certs = 1;
            break;

            case '2':
               opendcp->ca_cert_file = optarg;
               opendcp->xml_use_external_certs = 1;
            break;

            case '3':
               opendcp->signer_cert_file = optarg;
               opendcp->xml_use_external_certs = 1;
            break;

            case 'p':
               opendcp->private_key_file = optarg;
               opendcp->xml_use_external_certs = 1;
            break;

            case 'v':
               version();
            break;

            default:
               dcp_usage();
        }
    }

    /* set log level */
    dcp_set_log_level(opendcp->log_level);

    if (opendcp->log_level > 0) {
        printf("\nOpenDCP XML %s %s\n\n",OPENDCP_VERSION,OPENDCP_COPYRIGHT);
    }

    if (reel_count < 1) {
        dcp_fatal(opendcp,"No reels supplied");
    }

    /* check cert files */
    if (opendcp->xml_sign && opendcp->xml_use_external_certs == 1) {
        FILE *tp;
        if (opendcp->root_cert_file) {
            tp = fopen(opendcp->root_cert_file,"rb");
            if (tp) {
                fclose(tp);
            } else {
                dcp_fatal(opendcp,"Could not read root certificate");
            }
        } else {
            dcp_fatal(opendcp,"XML digital signature certifcates enabled, but root certificate file not specified");
        }
        if (opendcp->ca_cert_file) {
            tp = fopen(opendcp->ca_cert_file,"rb");
            if (tp) {
                fclose(tp);
            } else {
                dcp_fatal(opendcp,"Could not read ca certificate");
            }
        } else {
            dcp_fatal(opendcp,"XML digital signature certifcates enabled, but ca certificate file not specified");
        }
        if (opendcp->signer_cert_file) {
            tp = fopen(opendcp->signer_cert_file,"rb");
            if (tp) {
                fclose(tp);
            } else {
                dcp_fatal(opendcp,"Could not read signer certificate");
            }
        } else {
            dcp_fatal(opendcp,"XML digital signature certifcates enabled, but signer certificate file not specified");
        }
        if (opendcp->private_key_file) {
            tp = fopen(opendcp->private_key_file,"rb");
            if (tp) {
                fclose(tp);
            } else {
                dcp_fatal(opendcp,"Could not read private key file");
            }
        } else {
            dcp_fatal(opendcp,"XML digital signature certifcates enabled, but private key file not specified");
        }
    }
  
    /* set aspect ratio override */
    if (width || height) {
        if (!height) {
            dcp_fatal(opendcp,"You must specify height, if you specify width");
        }

        if (!width) {
            dcp_fatal(opendcp,"You must specify widht, if you specify height");
        }

        sprintf(opendcp->xml.aspect_ratio,"%d %d",width,height);
    }

    /* add pkl to the DCP (only one PKL currently support) */
    add_pkl(opendcp);

    /* add cpl to the DCP/PKL (only one CPL currently support) */
    add_cpl(opendcp, &opendcp->pkl[0]);

    /* Add and validate reels */
    for (c = 0;c<reel_count;c++) {
        if (add_reel(opendcp, &opendcp->pkl[0].cpl[0], reel_list[c]) != DCP_SUCCESS) {
            sprintf(buffer,"Could not add reel %d to DCP\n",c+1); 
            dcp_fatal(opendcp,buffer);
        }
       if (validate_reel(opendcp, &opendcp->pkl[0].cpl[0], c) != DCP_SUCCESS) {
            sprintf(buffer,"Could validate reel %d\n",c+1); 
            dcp_fatal(opendcp,buffer);
       }
    }

    /* Write XML Files */
    if (write_cpl(opendcp, &opendcp->pkl[0].cpl[0]) != DCP_SUCCESS)
        dcp_fatal(opendcp,"Writing composition playlist failed");
    if (write_pkl(opendcp, &opendcp->pkl[0]) != DCP_SUCCESS)
        dcp_fatal(opendcp,"Writing packing list failed");
    if (write_volumeindex(opendcp) != DCP_SUCCESS)
        dcp_fatal(opendcp,"Writing volume index failed");
    if (write_assetmap(opendcp) != DCP_SUCCESS)
        dcp_fatal(opendcp,"Writing asset map failed");

    dcp_log(LOG_INFO,"DCP Complete");

    if (opendcp->log_level > 0) {
        printf("\n");
    }

    delete_opendcp(opendcp);

    exit(0);
}
