#!/usr/bin/python

import os

def make(conn):

    # We must have a dummy empty cinema for these devices so that
    # we can use an INNER JOIN to search the database (as I can't
    # get LEFT OUTER JOIN to work).
    c = conn.cursor()
    c.execute("INSERT INTO cinema VALUES ('', '', '')")
    cinema_id = c.lastrowid

    for root, dirs, files in os.walk('doremi'):
        for filename in files:
            if filename.endswith('.cert.sha256.pem'):
                certificate = filename[filename.find('/')+1:]
                s = filename.split('-')
                device = s[0]
                serial_number = s[1].split('.')[0]
                c.execute("INSERT INTO device VALUES ('Doremi', '%s', '%s', '%s', '', '%d')" % (device, serial_number, certificate, cinema_id))

    conn.commit()
