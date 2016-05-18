#!/usr/bin/python

from csv import reader as csvreader
from sys import argv

""" Simple script to convert the output of powermeter to current, voltage, and
power.
"""

def channelToCurrent(raw):
    """ Convert the raw values to current.  Modify this to fit to your
    calibration.
    """
    return 26.722 + (float(raw) * 0.9904)

def channelToVoltage(raw):
    """ Convert the raw values to voltage.  Modify this to fit to your
    calibration.
    """
    return 6.787 + (float(raw) * 1.236)

def main(argv):
    if len(argv) < 2:
        print "Usage: {0} <csv file>".format(argv[0])
        return

    filename = argv[1]
    csvfile = open(filename, "rb")
    csviter = csvreader(csvfile, delimiter=',')

    mintime = None

    for line in csviter:

        time = line[0]
        channelA = line[1]
        channelB = line[2]

        current = channelToCurrent(channelA)
        voltage = channelToVoltage(channelB)
        power = current * voltage

        if not mintime:
            mintime = int(time)
        offsettime = int(time) - mintime

        output = [time, channelA, channelB, current, voltage, power, offsettime]

        print ','.join([str(x) for x in output])

    csvfile.close()

if __name__ == '__main__':
    main(argv)
