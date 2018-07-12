#!/usr/bin/env python

import numpy as np
from scipy import signal
import sys
import matplotlib.pyplot as plt

def inverse_sinc_filter(n_taps, n_bands, shift):
    bands = np.linspace(0, 1, n_taps)[1:-1]
    bands = np.repeat(bands, 2)
    bands = np.append(bands, 1)
    bands = np.insert(bands, 0, 0)
    desired = 1./np.sinc(bands/2.)
    b = signal.firls(n_taps, bands, desired)

    w, h = signal.freqz(b)

    plt.subplot(2,1,1)
    plt.plot(w/np.pi, np.abs(h))
    plt.plot(bands, desired, '+', markevery=10)

    plt.subplot(2,1,2)
    plt.plot(b)
    plt.draw()

    b_fp = np.rint(np.power(2, 30)*b).astype(int)
    return b_fp

def main():
    n_taps = 63
    n_bands = n_taps * 2
    shift = 24
    b_fp = inverse_sinc_filter(n_taps, n_bands, shift)

    try:
        file = open('fir_coeff.h', 'w')
    except:
        sys.stderr.write('Failed to open fir_coeff.h')

    file.write('const int fir_shift = %d;\n' % shift)
    file.write('std::vector<int32_t> coeff = {\n')
    for val in b_fp:
        file.write('  %10d,\n' % val)
    file.write('};\n')
    file.close()
    plt.show()

if __name__ == '__main__':
    main()




