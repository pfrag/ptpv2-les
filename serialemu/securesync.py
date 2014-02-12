#!/usr/bin/python

########################################################################
########################################################################

import time
import sys

try:
	datafile = open(sys.argv[1]);
except:
	print "Error: Could not file"
	sys.exit(1)


while 1:
	# read a line from file
	line = datafile.readline()
	sys.stdout.write(line.replace("\n", "\r\n"))
	time.sleep(1)

