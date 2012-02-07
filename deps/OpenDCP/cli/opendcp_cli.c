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
#include "win32/opendcp_win32_dirent.h"
#else
#include <dirent.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

#include "opendcp.h"

#ifndef WIN32
#define strnicmp strncasecmp
#endif

extern int build_filelist(char *input, char *output, filelist_t *filelist, int file_type);
extern int get_file_count(char *path, int file_type);

int check_extension(char *filename, char *pattern) {
    char *extension;

    extension = strrchr(filename,'.');

    if ( extension == NULL ) {
        return 0;
    }

    extension++;

   if (strnicmp(extension,pattern,3) !=0) {
       return 0;
   }

   return 1;
}

char *get_basename(const char *filename) {
    char *extension;
    char *base = 0;

    base = (char *)filename;
    extension = strrchr(filename,'.');
    base[(strlen(filename) - strlen(extension))] = '\0';

    return(base);
}

static int filter;
int file_filter(struct dirent *filename) {
    char *extension;

    extension = strrchr(filename->d_name,'.');

    if ( extension == NULL ) {
        return 0;
    }

    extension++;

    /* return only known asset types */
    if (filter == MXF_INPUT) {
        if (strnicmp(extension,"j2c",3) != 0 &&
            strnicmp(extension,"j2k",3) != 0 &&
            strnicmp(extension,"wav",3) != 0)
        return 0;
    } else if (filter == J2K_INPUT) {
        if (strnicmp(extension,"tif",3) != 0 &&
            strnicmp(extension,"dpx",3) != 0)
        return 0;
    }

    return 1;
}

int get_file_count(char *path, int file_type) {
    struct dirent **files;
    struct stat st_in;

    int x,count;

    filter = file_type;

    if (stat(path, &st_in) != 0 ) {
        dcp_log(LOG_ERROR,"Could not open input file %s",path);
        return DCP_FATAL;
    }

    if (S_ISDIR(st_in.st_mode)) {
        count = scandir(path,&files,(void *)file_filter,alphasort);
        for (x=0;x<count;x++) {
            free(files[x]);
        }
        free(files);

    } else {
        count = 1;
    }

    return count;
}

int build_filelist(char *input, char *output, filelist_t *filelist, int file_type) {
    struct dirent **files;
    int x = 0;
    struct stat st_in;

    if (stat(input, &st_in) != 0 ) {
        dcp_log(LOG_ERROR,"Could not open input file %s",input);
        return DCP_FATAL;
    }

    filelist->file_count = scandir(input,&files,(void *)file_filter,alphasort);
    if (filelist->file_count) {
        for (x=0;x<filelist->file_count;x++) {
            sprintf(filelist->in[x],"%s/%s",input,files[x]->d_name);
            if (file_type == J2K_INPUT) {
                sprintf(filelist->out[x],"%s/%s.j2c",output,get_basename(files[x]->d_name));
            }
        }
     }
    for (x=0;x<filelist->file_count;x++) {
        free(files[x]);
    }
    free(files);

    return DCP_SUCCESS;
}

filelist_t *filelist_alloc(int count) {
    int x;
    filelist_t *filelist;

    filelist = malloc(sizeof(filelist_t));

    filelist->file_count = count;
    filelist->in  = malloc(filelist->file_count*sizeof(char*));
    filelist->out = malloc(filelist->file_count*sizeof(char*));

    if (filelist->file_count) {
        for (x=0;x<filelist->file_count;x++) {
                filelist->in[x]  = malloc(MAX_FILENAME_LENGTH*sizeof(char *));
                filelist->out[x] = malloc(MAX_FILENAME_LENGTH*sizeof(char *));
        }
    }

    return filelist;
}

void filelist_free(filelist_t *filelist) {
    int x;

    for (x=0;x<filelist->file_count;x++) {
        if (filelist->in[x]) {
            free(filelist->in[x]);
        }
        if (filelist->out[x]) {
            free(filelist->out[x]);
        }
    }

    if (filelist->in) {
        free(filelist->in);
    }

    if (filelist->out) {
        free(filelist->out);
    }

    if (filelist) {
        free(filelist);
    }

    return;
}

int find_seq_offset(char str1[], char str2[]) {
    int i;
    int offset = 0;

    for (i = 0; (i < strlen(str1)) && (offset == 0); i++) {
        if(str1[i] != str2[i])
            offset = i;
    }

    return offset;
}

int find_ext_offset(char str[]) {
    int i = strlen(str);
    while(i) {
        if(str[i] == '.')
            return i;
        i--;
    }

    return 0;
}

int check_increment(char *str[], int index,int str_size) {
    long x;
    int seq_offset, ext_offset;

    seq_offset = find_seq_offset(str[0], str[str_size-1]);
    ext_offset = find_ext_offset(str[0]);

    x = strtol(str[index]+seq_offset,NULL,10);

    if (x == index) {
        return 0;
    } else {
        return 1;
    }
}

/* check if two strings are sequential */
int check_sequential(char str1[],char str2[]) {
    long i,x,y;
    int  offset = 0;

    if (strlen(str1) != strlen(str2)) {
        return STRING_LENGTH_NOTEQUAL;
    }

    for (i = 0; i < strlen(str1); i++) {
        if(str1[i] != str2[i]) {
            offset = i;
            break;
        }
    }

    x = strtol(str1+offset,NULL,10);
    y = strtol(str2+offset,NULL,10);

    if ((y - x) == 1) {
        return DCP_SUCCESS;
    } else {
        return STRING_NOTSEQUENTIAL;
    }
}

int check_file_sequence(char *str[], int count) {
    int sequential = 0;
    int x = 0;

    while (x<(count-1) && sequential == DCP_SUCCESS) {
        sequential = check_sequential(str[x], str[x+1]);
        x++;
    }

    if (sequential == DCP_SUCCESS) {
        return 0;
    } else {
        return x+DCP_ERROR_MAX;
    }
}
