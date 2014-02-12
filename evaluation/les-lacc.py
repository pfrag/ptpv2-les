#!/usr/bin/python

import random
import sys
import time
import scipy
import scipy.stats
import numpy
import math
import string

########################################################################
########################################################################
# Return average of window samples
def windowAverage(window):
	return numpy.average(window)	

def movingAverage(cur, s, b):
	return b*cur + (1-b)*s

def windowMovingAverage(window, b):
	avg = window[0]
	for i in range(len(window)):
		avg = b*avg + (1.0-b)*window[i]
	return avg

# Get (and return) a delay sample from file f (should already be open)
# ll: low load threshold: values < ll considered zero
def getSample(f, ll):
	try:
		line = f.readline()
		if line == "":
			#eof
			return None, None
		tokens = line.split(" ")
		if len(tokens) != 4:
			return -1.0, -1.0
	except:
		return -1.0, -1.0

	# tokens[0]: actual load
	# tokens[1]: delay
	if (float(tokens[0]) <= ll): 
		return 0.0, float(tokens[1])
#		return float(tokens[0]), float(tokens[1])
	else:
		return float(tokens[0]), float(tokens[1])

# fit for low loads
# load = (delay - b) / a
def fit(d):
	return (d - beta)/alpha

def expfit(d):
	return 0.8961*math.exp(4.656e-008*d) - 1.954*math.exp(-8.066e-006*d)

# [NOT USED] Detect alarm state. Compares the avg of the samples at the beginning and
# the end of the window
def checkAlarmState(window):
	start = window[0:5]
	end = window[windowsize - 5:windowsize - 1]
	avg1 = numpy.average(start)
	avg2 = numpy.average(end)
	if (avg2 > avg1 + margin):
		return True
	return False

# Test if the window is full (to start doing stuff)
def windowFull(window):
	for x in window:
		if x is None:
			return False
	return True
########################################################################
########################################################################

if len(sys.argv) < 3:
	print "Error: Wrong number of arguments."
	sys.exit(1)

# file to read samples from
samplefile = sys.argv[1]

# mode [0: ptpd, 1: securesync]
try:
	mode = int(sys.argv[2])
except:
	print "Error: Check the 2nd argument."
	sys.exit(1)

# window of samples
windowsize = 1
window = [None] * windowsize

# congestion threshold
threshold = 750000

# actual load of < 0.2 is considered 0
# Note: for 2-pair experiments, 10M --> 0.2 load
loadlow = 9000000

# [NOT USED] margin when checking for alarm state
margin = 100000

# average standard deviation of the window
avgstd = 0

# Select fitted curve
if mode == 1:
	#beta = 157163.0
	#alpha = 111103.0

	##no holdover, ptp devices, points 0-94
	beta = 104964.155
	alpha = 214373.634
	threshold_low = 150000

	##no holdover, ptp devices, only DREQ entries, points 0-94
	beta = 102175.03
	alpha = 209919.385
	threshold_low = 150000

	##no holdover, ptp devices, only DREQ entries, points 0.2-0.94
	beta = 91263.22
	alpha = 226779.66
	threshold_low = 150000

	# holdover, ptp devices
#original curve
	beta = 95142.640
	alpha = 228463.405
	threshold_low = 220000

	# new curve based on points 0.2-0.95
#	beta = 242636.92
#	alpha = 84302.52
#	threshold_low = 140000

	# holdover, ptpdev, 0.22-0.94, only DREQ
#	beta = 243657.49
#	alpha = 83252.43
#	threshold_low = 160000

	# no holdover, exp fit
	threshold_low = 145000

else:
	# ptpd
	beta = 485248.27
	alpha = 156104.27 
	threshold_low = 550000

# Open file with delay samples
try:
	f = open(samplefile, "r")
except:
	print "Error: Could not open samples file."
	sys.exit(1)

# sum of current squared errors
sumerror = 0
nsamples = 0

# current load estimate
curestimate = 0.0

while 1:
	# get delay sample
	# load < loadlow == zeroload
	load, sample = getSample(f, loadlow)
	if load is None:
		break

	if sample == -1.0:
		continue

	# /100 for 1-pair experiments
	# /50 for 2-pair experiments
	#load = load/50.0
	load = load/50000000.0
	# load > 1 == congestion
	if load > 1.0:
		load = 1.0

	# slide window
	window.pop()
	window.insert(0, sample)

	# if window not yet full, do nothing
	if not windowFull(window):
		avg = sample
		continue

	# estimate load
	# get delay avg
	win = windowAverage(window)
	avg = win
	
	#avg = movingAverage(avg, sample, 0.95)
	#avg = windowMovingAverage(window, 0.95)

	# test threshold
	if avg > threshold:
		# high load
		estimated = 1.0
	elif avg < threshold_low:
		estimated = 0.0
	else:
		# below congestion state, above low load area
		# use linear fit to detect exact low-load value
		if (expfit(avg) > 1):
			estimated = 1.0
		elif (expfit(avg) < 0):
			estimated = 0.0
		else:
			estimated = expfit(avg)
	
	curestimate = movingAverage(curestimate, estimated, 0.85)
	sumerror = sumerror + math.pow(curestimate - load, 2)
	#sys.stderr.write(str(curestimate) + " - " + str(load) + " = " + str(curestimate - load) + "\t" + str(math.pow(curestimate - load, 2)) + "\n")
	nsamples = nsamples + 1
	print str(avg) + "\t" + str(curestimate) + "\t" + str(load)
sys.stderr.write(str(sumerror/nsamples) + "\n")

