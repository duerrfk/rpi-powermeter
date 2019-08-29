#!/usr/bin/python

from csv import reader as csvreader
from sys import argv
from numpy import median

""" Use a median filter on the current and voltage in order to get a less noisy
power estimation.
"""

def main(argv):
    if len(argv) < 3:
        print 'Usage: {0} <filename> <window size>'.format(argv[0])
        return

    filename = argv[1]
    window_size = int(argv[2])

    csvfile = open(filename, 'rb')
    csviter = csvreader(csvfile, delimiter = ',')

    current_win = []
    voltage_win = []
    time_win = []

    for line in csviter:
        if len(line) < 7:
            print 'use add_power.py before'
            return

        time = int(line[0])
        channelA = int(line[1])
        channelB = int(line[2])

        current = float(line[3])
        voltage = float(line[4])
        power = float(line[5])
        offsettime = int(line[6])

        if len(current_win) >= window_size:
            current_med = median(current_win)
            voltage_med = median(voltage_win)
            time_med = median(time_win)

            new_current_win = current_win[1:]
            new_voltage_win = voltage_win[1:]
            new_time_win = time_win[1:]

            current_win = new_current_win
            voltage_win = new_voltage_win
            time_win = new_time_win

            power_med = current_med * voltage_med

            output = [time_med, current_med, voltage_med, power_med]

            print ','.join(str(x) for x in output)

        current_win.append(current)
        voltage_win.append(voltage)
        time_win.append(offsettime)

    csvfile.close()

if __name__ == '__main__':
    main(argv)
