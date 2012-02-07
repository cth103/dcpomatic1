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

#ifndef OPENDCP_CLI_H
#define OPENDCP_CLI_H

int check_extension(char *filename, char *pattern);
char *get_basename(const char *filename);
int get_file_count(char *path, int file_type);
filelist_t *filelist_alloc(int count);
void filelist_free(filelist_t *filelist);
int find_ext_offset(char str[]);
int find_seq_offset (char str1[], char str2[]);
int check_increment(char *str[], int index,int str_size);
int check_sequential(char str1[],char str2[]);
int check_file_sequence(char *str[], int count);
int build_filelist(char *input, char *output, filelist_t *filelist, int file_type);

#endif
