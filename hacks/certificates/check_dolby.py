import os
import sqlite3
import zipfile

def get_cert(zip):
    try:
        z = zipfile.ZipFile('dolby/' + zip)
        return z.open(z.infolist()[1].filename).read()
    except:
        pass

db = sqlite3.connect('certificates.db')
c = db.cursor() 
for s in range(510000, 550000):
    c.execute("SELECT * FROM device WHERE serial='%d'" % s)
    n = 0
    rows = []
    while True:
        r = c.fetchone()
        if r is None:
            break
        rows.append(r)

    if len(rows) >= 2:
        print '->'
        print rows[0][3]
        print rows[1][3]
        if get_cert(rows[0][3]) == get_cert(rows[1][3]):
            print 'Certs same'
        else:
            print 'Certs differ'

