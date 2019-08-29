#!/usr/bin/python

from csv import reader as csvreader
from sys import argv

""" Script to integrate the power of a given time frame and output energy.
"""

def main(argv):
    if len(argv) < 4:
        print "Usage: {0} <csv file> <start> <end>".format(argv[0])
        return

    filename = argv[1]
    tstart = int(argv[2])
    tend = int(argv[3])

    csvfile = open(filename, "rb")
    csviter = csvreader(csvfile, delimiter = ',')

    last_time = None
    last_power = None

    energy = 0.0

    for line in csviter:

        if len(line) < 6:
            print 'run add_power.py before'
            return

        time_orig = int(line[0])
        channel_a = int(line[1])
        channel_b = int(line[2])

        current = float(line[3])
        voltage = float(line[4])
        power = float(line[5])
        time = int(line[6])

        if tstart <= time and time <= tend:
            if last_time is not None:
                timeframe = time - last_time
                energy += (timeframe * (last_power + power)) / 2.0

            last_time = time
            last_power = power

    csvfile.close()

    print 'energy: {0}'.format(energy / 1000000000000000)

if __name__ == '__main__':
    main(argv)

