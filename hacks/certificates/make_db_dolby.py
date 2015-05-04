#!/usr/bin/python
# -*- coding: UTF-8 -*-

import sqlite3
import os
import re
import sys
import zipfile

ignore_files = ['Thumbs.db', 'removed.zip', 'Invalid.zip', 'Removed.zip', 'replaced.zip', 'old3.zip',
                '.FDF', 'ShowPlayer_Install_4.0.6.13_DC256.Cinea.Com_2010-06-30-09-28-36_KDMs.zip',
                'CAT86x_Install_4.3.0.11_DC256.Cinea.Com_2013-10-22-05-38-33_KDMs.zip',
                'CAT86x_Install_4.4.0.50_DC256.Cinea.Com_2013-01-24-15-50-50_KDMs_Lonsdale_Cinema_Annan.zip',
                'CAT86x_First_Install_4.4.0.46_DC256.Cinea.Com_2013-01-24-15-50-28_KDMs_Lonsdale_Cinema_Annan.zip',
                'CAT86x_Install_4.4.0.50_DC256.Cinea.Com_2013-02-05-14-35-11_KDMs_MediCinema_Serennu_Children’s_Ctr_Newport.zip',
                'Saint-Petersburg_State_University_of_Cinema_CAT86x_Install_4.3.0.9_DC256.Cinea.Com_2012-02-08-14-07-10_KDMs.zip',
                'Megaplex_Cevahir_Sisli_CAT86x_Install_4.3.0.9_DC256.Cinea.Com_2011-05-16-10-17-48_KDMs.zip',
                'Apollo_Messina_CAT86x_Install_4.3.0.9_DC256.Cinea.Com_2011-11-21-08-22-36_KDMs.zip',
                'Mobile kit 1 - decommissioned.txt',
                'Screen 3.txt']

def read_device_info(n):
    s = n[n.find('cert_'):].split('_')
    p = None
    for i in s:
        if i.startswith('Dolby') and p is None:
            p = i.split('-')
    device = p[1]
    serial = p[2]
    if serial.startswith('H'):
        # I don't know what this H prefix means
        serial = serial[1:]

    serial = re.search('(\d*)', serial).group(0)

    try:
        serial_number = int(serial)
    except:
        serial_number = None

    return (device, serial_number)

def split_path(p):
    s = []
    while True:
        a, b = os.path.split(p)
        s.append(b)
        if len(a) == 0:
            break
        p = a

    s.reverse()
    return s

def escape(s):
    return s.replace("'", "''")

def make(conn):
    c = conn.cursor()
    c.execute("INSERT INTO manufacturer VALUES ('Dolby', 'ftp://dolbyrootcertificates:houro61l@ftp.dolby.co.uk/SHA256')")
    manufacturer = c.lastrowid
    conn.commit()

    for root, dirs, files in os.walk('dolby'):
        for filename in files:
            full = os.path.join(root, filename)
            if filename in ignore_files:
                continue

            if filename.endswith('.zip'):
                filename = filename[:-4]
            else:
                filename = filename

            device, serial_number = read_device_info(filename)
            cinema = ''
            if filename.startswith('cert_'):
                s = filename.split('_')
                for i in range(2, len(s)):
                    cinema += s[i] + ' '
            else:
                s = filename[:filename.find('cert_')]
                for i in s.split('_'):
                    cinema += i + ' '

            cinema = cinema.strip()
            cinema = escape(cinema)
            s = cinema.split()
            if len(s) > 0 and s[len(s)-1].isdigit():
                screen = s[len(s)-1]
                cinema = cinema[:-(len(screen)+1)]
            else:
                screen = ''

            path = split_path(root)
            country = path[1]
            assert(device.startswith("CAT") or device.startswith("DSS") or device.startswith("DSP"))
            assert(serial_number is not None)

            certificate = escape(full[full.find('/')+1:])

            c = conn.cursor()
            c.execute("SELECT rowid FROM cinema WHERE name = '%s' AND country = '%s'" % (cinema, country)) 
            r = c.fetchone()
            if r is not None:
                cimema_id = r[0]
            else:
                c.execute("INSERT INTO cinema VALUES ('%s', '', '%s')" % (cinema, country))
                cinema_id = c.lastrowid

            c.execute("INSERT INTO device VALUES ('%d', '%s', '%s', '%s', '%s', '%d')" % (manufacturer, device, serial_number, certificate, screen, cinema_id))

