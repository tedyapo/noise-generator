#!/usr/bin/env python
#
# quick histogram of byte values in file
#

import array

data = array.array('b')

try:
    file = open('noise.dat')
except (IOError) as e:
    print("Can't open noise.dat {}".format(e.args[-1]))

while (True):
    try:
        data.fromfile(file, 1024)
    except (EOFError):
        break

histogram = [0] * 256
for datum in data:
    histogram[int(datum) + 128] += 1

for val in histogram:
    print val
