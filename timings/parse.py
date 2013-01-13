import time
import datetime
import os

pounds_per_dollar = 0.62

cost_per_hour = dict()
# Micro
cost_per_hour['t1.micro'] = 0.02
# High CPU medium
cost_per_hour['c1.medium'] = 0.186
# High CPU extra large
cost_per_hour['c1.xlarge'] = 0.744

class Data:
    def __init__(self):
        self.frames = None
        self.content = None
        self.source_width = None
        self.source_height = None
        self.start_time = None
        self.finish_time = None
        self.host = None
        self.notes = []
        self.threads = None
        self.j2k_bandwidth = None

    def duration(self):
        return datetime.timedelta(seconds = time.mktime(self.finish_time) - time.mktime(self.start_time))

    def duration_seconds(self):
        d = self.duration()
        return d.seconds + d.days * 24 * 3600

    def frames_per_second(self):
        return float(self.frames) / self.duration_seconds()

    def cost(self):
        if self.host is None or self.host not in cost_per_hour:
            return None

        return (self.duration_seconds() / 3600) * cost_per_hour[self.host]

    def dump(self):
        for n in self.notes:
            print n
        print 'Content: %s' % self.content
        if self.threads is not None:
            print 'Threads: %s' % self.threads
        if self.j2k_bandwidth is not None:
            print 'J2K bandwidth: %d' % (self.j2k_bandwidth / 1e6)
        print 'Host: %s' % self.host
        print 'Frames: %d' % self.frames
        print 'Source size: %dx%d' % (self.source_width, self.source_height)
        print 'Start: %s' % time.strftime('%a %b %d %H:%M:%S %Y', self.start_time)
        print 'Finish: %s' % time.strftime('%a %b %d %H:%M:%S %Y', self.finish_time)
        ds = self.duration_seconds()
        dh = ds / 3600
        print 'Log duration: %s' % self.duration()
        print 'Frames per second: %f' % self.frames_per_second()
        print 'Seconds per frame: %f' % (1 / self.frames_per_second())
        c = self.cost()
        if c is not None:
            print 'Cost: $%0.2f (%0.2f ukp)' % (c, c * pounds_per_dollar)

class Parser:
    def __init__(self, directory):
        self.directory = directory

    def parse(self):
        data = Data()

        metadata = open(os.path.join(self.directory, 'metadata'), 'r')
        for l in metadata.readlines():
            l = l.strip()
            n = l.find(' ')
            if n != -1:
                k = l[:n]
                v = l[n+1:]
                if k == 'length':
                    data.frames = int(v)
                elif k == 'content':
                    data.content = v
                elif k == 'width':
                    data.source_width = int(v)
                elif k == 'height':
                    data.source_height = int(v)

        log = open(os.path.join(self.directory, 'log'), 'r')
        for l in log.readlines():
            l = l.strip()
            event_time = time.strptime(l[:24], '%a %b %d %H:%M:%S %Y')
            message = l[26:]
            
            if message == 'Transcode job starting':
                data.start_time = event_time

            s = message.split()
            if len(s) == 2 and s[1] == 'threads':
                data.threads = int(s[0])
            elif len(s) == 3 and s[0] == 'J2K' and s[1] == 'bandwidth':
                data.j2k_bandwidth = int(s[2])

            data.finish_time = event_time

        try:
            notes = open(os.path.join(self.directory, 'notes'), 'r')
            for n in notes.readlines():
                data.notes.append(n.strip())
        except:
            pass

        return data

if __name__ == '__main__':
    for r in ['t1.micro/1', 'c1.xlarge/1', 'houllier/1']:
        print '-- %s' % r
        p = Parser(r)
        d = p.parse()
        d.host = r.split('/')[0]
        d.dump()
	print ''
