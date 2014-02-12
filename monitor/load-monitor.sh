#!/bin/bash

##########################################################################
# load-monitor.sh                                                        #
#                                                                        #
# This script reads delay-load info from a file generated by LES and     #
# plots the estimated load in real time.                                 #
# Example: ./load-monitor.sh fname                                       #
#                                                                        #
# Author: Pantelis A. Frangoudis [pantelis.frangoudis@inria.fr]          #
##########################################################################

# File with delay-load values (3 columns)
DATAFILE=$1

# Window of samples to plot
SAMPLEWIN=10000

# Y-axis
YMAX=1.0
YMIN=0.0

# Create a gnuplot script and execute it. Should end with the reread command.
cat > ./rt-load-plot.gp <<EOF
set title "Network load estimated from path delay measurements"
set key off
set xdata time
set timefmt "%Y-%m-%d,%H:%M:%S"
set xrange [0:$SAMPLEWIN]
set yrange [$YMIN:$YMAX]
set ylabel "Load"
set xlabel "Time"
# Uncomment to hide time from x-axis
set noxtics
set autoscale x
plot '< tail -n $SAMPLEWIN $DATAFILE ' using 1:3 w points
pause 1
reread
EOF

gnuplot ./rt-load-plot.gp
