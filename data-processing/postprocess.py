#!/usr/bin/python

########################################################################
# IPChronos experiment instrumentation.                                #
#                                                                      #
# This script processes experiment data as follows:                    #
# - For each file in the input folder, parse each line and identify    #
# the instances where load generation starts and stops, i.e.,          #
# "tx start/stop" strings.                                             #
# - Skip a number of samples after tx start and before tx stop         #
# - If the values of the last two columns do not change, skip this line#
# - Otherwise, write this line to a file                               #
#                                                                      #
########################################################################

import os
import shutil
import sys

SAMPLES_TO_SKIP = 10
logdir = sys.argv[1]
outputdir = os.path.dirname(logdir) + "-pp" + "/"

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

	# loop through lines of input until [tx stopped] is found
	sflag = 0
	skipped = SAMPLES_TO_SKIP
	cur4 = ""
	cur5 = ""
	while 1:
		line = fin.readline()
		if not line:
			break
		if not sflag:
			pos = line.find("[tx started]")
			if pos >= 0:
				sflag = 1
				continue
		else:
			pos = line.find("[tx stopped]")
			if pos >= 0:
				break

		# split line and check last two columns
		tokens = line.split(" ")
			
		if skipped == 0:
			# Not in skipping mode: Check if the values of the 2 last columns
			# change. If so, output line
			#if len(tokens) == 6 and (cur4 != tokens[4] or cur5 != tokens[5]) and (int(tokens[5]) - int(tokens[4]) + int(tokens[3]) - int(tokens[2]) > 0):
			if len(tokens) == 6 and (cur4 != tokens[4] or cur5 != tokens[5]):
				#if (int(tokens[5]) - int(tokens[4]) + int(tokens[3]) - int(tokens[2]) < 0):
				fout.write(line)
				cur4 = tokens[4]
				cur5 = tokens[5]
			else:
				#ignore line
				pass
		else:
			# we are in line skipping mode
			if len(tokens) == 6:
				skipped = skipped - 1

	# close files
	fin.close
	fout.close

