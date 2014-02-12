#!/usr/bin/python

########################################################################
# IPChronos experiment instrumentation.                                #
#                                                                      #
# This script works on post-processed data and calculates statistics   #
#                                                                      #
# Author: Pantelis A. Frangoudis [pantelis.frangoudis@inria.fr]        #
########################################################################

import scipy
import scipy.stats
import numpy
import sys
import os
import shutil
import string
import math

logdir = sys.argv[1]
outdir = "./results/" + os.path.dirname(logdir) + "/"

summary_m2s = "summary-m2s.csv"
summary_s2m = "summary-s2m.csv"
summary_b = "summary-b.csv"

# from the normal distro tabs, for 95% CI
zeta = 1.96 

# Create output directory
try:
	shutil.rmtree(outdir)
except:
#	print "Error: Could not remove output directory"
#	sys.exit(1)
	pass

try:
	os.mkdir(outdir)
except:
	print "Error: Could not create output directory"
	sys.exit(1)

# list files in input directory
files = os.listdir(logdir)
files.sort()

# summary files
fsum_s2m = open(outdir + summary_s2m, "w")
fsum_m2s = open(outdir + summary_m2s, "w")
fsum_b = open(outdir + summary_b, "w")

# for each file calculate:
# 1. Average one-way M2S delay & stdev
# 2. RTT/2 and stdev
# 3. CDF file
# 4. PDF file
for f in files:
	# open PDF & CDF files
	cdfout_t2t1 = open(outdir + f + ".t2t1.cdf", "w")
	pdfout_t2t1 = open(outdir + f + ".t2t1.pdf", "w")
	cdfout_h = open(outdir + f + ".h.cdf", "w")
	pdfout_h = open(outdir + f + ".h.pdf", "w")

	# open input file
	fdata = numpy.loadtxt(logdir + f, usecols = (2, 3, 4, 5));
	# T2-T1
	t2t1 = numpy.subtract(fdata[:,1], fdata[:,0]);
	# T4-T3
	t4t3 = numpy.subtract(fdata[:,3], fdata[:,2]);
	# RTT/2
	halfrtt = numpy.divide(numpy.add(t2t1, t4t3), 2.0)

	# calculate averages and output to file
	# filename	numsamples	avg(t2-t1)	std(t2-t1)	lowci	hici	avg(rrt/2)	std(rtt/2) lowci	hici
	lowt2t1 = numpy.average(t2t1) - zeta*numpy.std(t2t1)/math.sqrt(len(t2t1))
	hight2t1 = numpy.average(t2t1) + zeta*numpy.std(t2t1)/math.sqrt(len(t2t1))
	lowhalfrtt = numpy.average(halfrtt) - zeta*numpy.std(halfrtt)/math.sqrt(len(halfrtt))
	highhalfrtt = numpy.average(halfrtt) + zeta*numpy.std(halfrtt)/math.sqrt(len(halfrtt))

	if string.find(f, "S2M") > -1:
		fsum_s2m.write(f + "\t" + str(len(t2t1)) + "\t" + str(numpy.average(t2t1)) + "\t" + str(numpy.std(t2t1)) + "\t" + str(lowt2t1) + "\t" + str(hight2t1) + "\t" + str(numpy.average(halfrtt)) + "\t" + str(numpy.std(halfrtt)) + "\t" + str(lowhalfrtt) + "\t" + str(highhalfrtt) + "\n")
	elif string.find(f, "M2S") > -1:
		fsum_m2s.write(f + "\t" + str(len(t2t1)) + "\t" + str(numpy.average(t2t1)) + "\t" + str(numpy.std(t2t1)) + "\t" + str(lowt2t1) + "\t" + str(hight2t1) + "\t" + str(numpy.average(halfrtt)) + "\t" + str(numpy.std(halfrtt)) + "\t" + str(lowhalfrtt) + "\t" + str(highhalfrtt) + "\n")
	else:
		fsum_b.write(f + "\t" + str(len(t2t1)) + "\t" + str(numpy.average(t2t1)) + "\t" + str(numpy.std(t2t1)) + "\t" + str(lowt2t1) + "\t" + str(hight2t1) + "\t" + str(numpy.average(halfrtt)) + "\t" + str(numpy.std(halfrtt)) + "\t" + str(lowhalfrtt) + "\t" + str(highhalfrtt) + "\n")

	# distributions
	nb = 150
	cumfreqs, lowlim, binsize, extrapoints = scipy.stats.cumfreq(t2t1, numbins=nb)
	for val in cumfreqs:
		cdfout_t2t1.write(str(val) + "\n")
	cumfreqs, lowlim, binsize, extrapoints = scipy.stats.cumfreq(halfrtt, numbins=nb)
	for val in cumfreqs:
		cdfout_h.write(str(val) + "\n")

	densities, lowlim, binsize, extrapoints = scipy.stats.histogram(t2t1, numbins=nb)
	for val in densities:
		pdfout_t2t1.write(str(val) + "\n")
	densities, lowlim, binsize, extrapoints = scipy.stats.histogram(halfrtt, numbins=nb)
	for val in densities:
		pdfout_h.write(str(val) + "\n")

	# close files
	cdfout_t2t1.close
	pdfout_t2t1.close
	cdfout_h.close
	pdfout_h.close

fsum_s2m.close
fsum_m2s.close
fsum_b.close
