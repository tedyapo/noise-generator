# noise-generator
Flat-spectrum noise generation for use with osmo-fl2k and FL2k USB-to-VGA dongles

Use with osmo-fl2k like this:
```
./noisegen | fl2k_file -s 80000000 -
```
make sure you have an up-to-date copy of the fl2k_file source; there was a bug in early versions that didn't allow reading from standard input

select the sample rate with the -s parameter.  The noisgen program generates a flat-spectrum noise up to the Nyquist rate (1/2 the sample rate).

Speed Tests
```
./noisegen | pv -ab | dd bs=65536 count=32768 iflag=fullblock > /dev/null
```

Output Histogram
```
noisegen | dd bs=65536 count=32 > noise.dat
./histogram.py > histo.dat
```
