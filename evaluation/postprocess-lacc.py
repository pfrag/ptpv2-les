#!/usr/bin/python

########################################################################
# IPChronos experiment instrumentation.                                #
#                                                                      #
# This script processes experiment data as follows:                    #
# - For each file in the input folder, parse each line and identify    #
# the instances where load generation starts and stops, i.e.,          #
# "tx start/stop" strings.                                             #
# - Skip a number of samples after tx start and before tx stop         #
# - Write this line to a file                                          #
#                                                                      #
# This version works on data received from spectracom devices and tags #
# each output line with the current load value                         #
########################################################################

import os
import shutil
import sys
import string

logdir = sys.argv[1]
outputdir = os.path.dirname(logdir) + "-pp-lacc/"

# assumes that "tx started" has been already detected in the string
def parseLoadValue(line):
	pos1 = line.find("-")
	pos2 = line.find("]")
	retval = line[pos1 + 1:pos2] #watch out: if ends in M, need pos2 - 1
	return int(retval)


# Create output directory
try:
	shutil.rmtree(outputdir)
except:
	pass

try:
	os.mkdir(outputdir)
except:
	print "Error: Could not create output directory"
	sys.exit(1)

# list files in input directory
files = os.listdir(logdir)

# for each file, do the parsing and output a file with the same name inside outputdir
for f in files:
	# open output file
	fout = open(outputdir + f + ".csv", "w")

	# open input file
	fin = open(logdir + f, "r")

	# loop through lines of input
	curload = 0
	cur4 = ""
	cur5 = ""
	while 1:
		line = fin.readline()
		if not line:
			break

		pos = line.find("[tx started-")
		if pos >= 0:
			curload = parseLoadValue(line)
			continue

		pos = line.find("[tx stopped]")
		if pos >= 0:
			# reset current load value
			curload = 0
			continue

		# split line and check 3rd column
		tokens = line.split(" ")
			
		# Output line (if not malformed)
		#if len(tokens) == 6:# and (cur4 != tokens[4] or cur5 != tokens[5]):
		if len(tokens) == 6 and (cur4 != tokens[4] or cur5 != tokens[5]):
			# mpd, m2s, s2m
			mpd = (int(tokens[3]) - int(tokens[2]) + int(tokens[5]) - int(tokens[4]))/2.0
			m2s = int(tokens[3]) - int(tokens[2])
			s2m = int(tokens[5]) - int(tokens[4])
#			if mpd > 0.0:
			fout.write(str(curload) + " " + str(mpd) + " " + str(m2s) + " " + str(s2m) + "\n")
			cur4 = tokens[4]
			cur5 = tokens[5]
		else:
			#ignore line
			pass

	# close files
	fin.close
	fout.close

