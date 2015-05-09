#!/usr/bin/python
# -*- coding: UTF-8 -*-

import sqlite3
import os
import re
import sys
import zipfile

import make_db_dolby
import make_db_doremi

try:
    os.unlink('certificates.db')
except:
    pass

conn = sqlite3.connect('certificates.db')
c = conn.cursor()

c.execute("CREATE VIRTUAL TABLE manufacturer USING fts3 (name, url)")
c.execute("CREATE VIRTUAL TABLE device USING fts3 (manufacturer, model, serial, certificate, screen, cinema)")
c.execute("CREATE VIRTUAL TABLE cinema USING fts3 (name, email, country)")
conn.commit()

make_db_dolby.make(conn)
make_db_doremi.make(conn)

