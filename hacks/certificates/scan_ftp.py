#!/usr/bin/python
#
# Read Dolby and Doremi FTP sites into text files
# `dolby.dump' and `doremi.dump'
#

import os
import re
from ftplib import FTP
import random
import time

filename_position = 55

# Read a FTP directory, returning directories and files
def read(ftp, dir):
    wait = random.randint(1, 10)
    print '  Read directory %s (after %ds)' % (dir, wait)
    time.sleep(wait)
    ftp.cwd(dir)
    lines = []
    ftp.retrlines('LIST', lines.append)

    dirs = []
    files = []
    for l in [l for l in lines if len(l) > filename_position]:
        f = l[filename_position:]
        if f == '.' or f == '..':
            continue

        if l[0] == 'd':
            dirs.append(f)
        else:
            files.append(f)
            
    return dirs, files

# Recursively scan a FTP directory.
def scan(ftp, dir, all_files):
    print ' Scanning %s; %d files found' % (dir, len(all_files))
    dirs, files = read(ftp, dir)
    for d in dirs:
        scan(ftp, dir + '/' + d, all_files)
    for f in files:
        all_files.append(dir + '/' + f)

def read_site(site, user, password, prefix, output):
    print 'Site %s' % site
    ftp = FTP(site, user, password)
    all_files = []
    scan(ftp, prefix, all_files)
    o = open(output, 'w')
    for f in all_files:
        print>>o,f

#
# Dolby
#
# First, read countries, making local directories for each one.
#

ftp = FTP('ftp.dolby.co.uk', 'dolbyrootcertificates', 'houro61l')
countries = 
dirs, files = scan(ftp, '/SHA256', al

, '/SHA256', 'dolby.dump')
read_site('ftp.doremilabs.com', 'service', 't3chn1c1an', '/Certificates', 'doremi.dump')
