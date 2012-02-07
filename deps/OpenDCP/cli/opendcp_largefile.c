#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <inttypes.h>
#include <time.h>
#include <sys/stat.h>
#include "opendcp.h"

int main (int argc, char **argv)
{
    struct stat st;
 
    if (argc<2) {
        printf("\nTest to see if large file support is enabled. Should return correct\n");
        printf("size of files larger than 4gb\n\n");
        printf("usage: opendcp_largefile <file>\n");
        
        return -1;
    }

    char *filename = argv[1];
    stat(filename, &st);
    printf("file: %s size: %"PRIu64 "\n",filename,st.st_size);

    return 0;
}
