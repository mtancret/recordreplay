#!/usr/bin/env python
#
# Usage:
# collect.py filename

import time
import os
from serial import *

#fileNameBase = sys.argv[1]
fileNameBase = 'build/log'
genFileName = fileNameBase + '-gen.dat'
stateFileName = fileNameBase + '-state.dat'
intFileName = fileNameBase + '-int.dat'
valuesFileName = fileNameBase + '-values.dat'
countsFileName = fileNameBase + '-counts.dat'
errFileName = fileNameBase + '-err.dat'
genFile = open(genFileName, 'w')
stateFile = open(stateFileName, 'w')
intFile = open(intFileName, 'w')
valuesFile = open(valuesFileName, 'w')
countsFile = open(countsFileName, 'w')
errFile = open(errFileName, 'w')

ser = Serial(port=os.getenv("MOTEDEV"), baudrate=115200, timeout=None)
ser.flushInput()

pageCount = 0
genPageCount = 0
statePageCount = 0
intPageCount = 0
valuesPageCount = 0
countsPageCount = 0
errPageCount = 0

data = []

while (ser.read (1) != 'r'):
	""" """
ser.read (2)
while(1):
	# MSP430 is little-endian
	headerHigh = ord (ser.read (1))
	headerLow = ord (ser.read (1))

	print "page header", headerHigh, headerLow

	if (headerLow == 0 and headerHigh == 0):
		break

	if headerLow & 0xf == 0:
		genPageCount += 1
		targetFile = genFile
		print "gen page"
	elif headerLow & 0xf == 1:
		statePageCount += 1
		targetFile = stateFile
		print "state page"
	elif headerLow & 0xf == 2:
		intPageCount += 1
		targetFile = intFile
		print "int page"
	elif headerLow & 0xf == 3:
		valuesPageCount += 1
		targetFile = valuesFile
		print "values page"
	elif headerLow & 0xf == 4:
		countsPageCount += 1
		targetFile = countsFile
		print "counts page"
	else:
		errPageCount += 1
		targetFile = errFile
		print "ERROR: unknown page type"

	if headerLow & 0xf0 == 0x10:
		errPageCount += 1
		targetFile = errFile
		print "ERROR: buff not ready"
	elif headerLow & 0xf0 == 0x20:
		errPageCount += 1
		targetFile = errFile
		print "ERROR: bit pack overflow"
	elif headerLow & 0xf0 == 0x40:
		print "WARNING: flash busy"
	elif headerLow & 0xf0 == 0x80:
		errPageCount += 1
		targetFile = errFile
		print "ERROR: compressor busy"

	size = headerHigh
	for i in range (254/2):
		valueLow = ser.read (1)
		valueHigh = ser.read (1)
		if i*2 < size:
			data.append (ord(valueHigh))
			data.append (ord(valueLow))
			targetFile.write (valueHigh)
			targetFile.write (valueLow)

	pageCount += 1
	ser.read (2)

print data
print "Num pages", pageCount
print "Gen pages", genPageCount
print "State pages", statePageCount
print "Int pages", intPageCount
print "Values pages", valuesPageCount
print "Counts pages", countsPageCount
print "Err pages", errPageCount

