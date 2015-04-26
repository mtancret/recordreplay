#!/usr/bin/env python
# Read output files genearted by gcc binutils 'as' and 'ld' and creates a file address-map.out.
# The output file indiates the address locations of tscribe instrumentation.
import re
import itertools
import sys

path = sys.argv[1]

asFileName = path+"/as.out"
ldFileName = path+"/ld.out"
objdumpFileName = path+"/objdump.out"
binMapFileName = path+"/address-map.out"
binMapFile = open(binMapFileName, "w")

# fileId:int -> fileName:string
files = {}
# functionName:string -> func:Func
functions = {}
# varName:string -> address:int
globalVars = {}

# Regular Expressions
#  48               		.file 2 "clock.c"
reFile = re.compile("\s+\d+\s+\.file\s+\d+")
#  49               		.loc 2 11 0
reLoc = re.compile("\s+\d+\s+\.loc\s+\d+")
#  42               		.type	tscribe_max_dco,@function
reFunc = re.compile("\s+\d+\s+\.type\s+[A-Za-z_][A-Za-z_0-9]*,@function")
#  58 0036 F240 E0FF 		mov.b	#llo(-32), &__DCOCTL
reInst = re.compile("\s+\d+\s+[0-9A-Fa-f]{4}\s+[0-9A-Fa-f]{4}")
#  2576               	;; End of function 
reEndFunc = re.compile("\s+\d+\s+\;\; End of function")
#                0x000000000000405c                _endless_loop__
reGlobAddr = re.compile("\s+0x[0-9A-Fa-f]{16}\s+[A-Za-z_][A-Za-z_0-9]*")
# 00004214 <tscribe_flash_program_page>:
reFuncObjdump = re.compile("[0-9A-Fa-f]{8}\s+\<[A-Za-z_][A-Za-z_0-9]*\>\:")

# Location maps a source file location to a list of addresses in the binary
class Location:
	def __init__(self, fileId, lineNum, charNum):
		self.fileId = fileId
		self.lineNum = lineNum
		self.charNum = charNum
		self.addresses = []

	def printAddresses(self):
		print "[",
		for addr in self.addresses:
			print hex(addr),",",
		print "]"

	def firstAddr(self):
		if self.addresses == []:
			return None
		else:
			return self.addresses[0]

	def isEmpty(self):
		return len(self.addresses) == 0

	def addAddr(self, addr):
		self.addresses.append(addr)

	def translateAddresses(self, offset):
		for i in range(len(self.addresses)):
			self.addresses[i] += offset	

# Func contains a list of Location
class Func:
	def __init__(self, name):
		self.name = name

		self.name = name
		self.locations = []

	def firstLoc(self):
		if self.locations == []:
			return None
		else:
			return self.locations[0]

	def printLocations(self):
		for loc in self.locations:
			loc.printAddresses()
		print

	def addLoc(self, loc):
		self.locations.append(loc)

	def firstAddr(self):
		for loc in self.locations:
			firstAddr = loc.firstAddr()
			if firstAddr != None:
				return firstAddr
		return None

	def lastAddr(self):
		lastAddr = None
		for loc in self.locations:
			if loc.firstAddr() != None:
				lastAddr = loc.firstAddr()
		return lastAddr

	def translateAddresses(self, offset):
		for loc in self.locations:
			loc.translateAddresses(offset)

# Parse the file generate by 'as' when run with the command options '-alh=file -S'.
def parseAs():
	asFile = open(asFileName)
	currentLoc = None
	currentFunc = None
	line = asFile.readline()
 	while line:
		if reFile.match(line) != None:
			parts = line.split()
			fileId = int(parts[2])
			fileName = parts[3][1:-1]
			files[fileId] = fileName
		elif reFunc.match(line) != None:
			parts = line.split()
			funcName = parts[2].split(",")[0]
			currentFunc = Func(funcName)
			functions[funcName] = currentFunc
		elif reLoc.match(line) != None:
			parts = line.split()
			fileId = int(parts[2])
			lineNum = int(parts[3])
			charNum = int(parts[4])
			currentLoc = Location(fileId, lineNum, charNum)
			if currentFunc != None:
				currentFunc.addLoc(currentLoc)
				#print "loc: ", fileId, lineNum, charNum
		elif reInst.match(line) != None:
			parts = line.split()
			addr = int(parts[1], 16)
			if currentLoc != None:
				currentLoc.addAddr(addr)
				#print "addr: ", addr
		elif reEndFunc.match(line) != None:
			#currentFunc.printLocations()
			currentFunc = None
			currentLoc = None
			#print "End of Function"

		line = asFile.readline()

	asFile.close()

# Parse the map file generate by ld when run with the -M=mapfile flag.
def parseLd():
	ldFile = open(ldFileName)
	line = ldFile.readline()
 	while line:
		if reGlobAddr.match(line) != None:
			parts = line.split()
			addr = int(parts[0], 16)
			globName = parts[1]
			globalVars[globName] = addr

		line = ldFile.readline()
	ldFile.close()

# Parse the file generate by objdump.
def parseObjdump():
	objdumpFile = open(objdumpFileName)
	line = objdumpFile.readline()
 	while line:
		if reFuncObjdump.match(line) != None:
			parts = line.split()
			addr = int(parts[0], 16)
			funcName = parts[1][1:-2]
			if functions.has_key(funcName):
				func = functions[funcName]
				startAddr = func.firstAddr()
				if startAddr != None:
					offset = addr - startAddr
					#print "name:",funcName,"starAddr:",startAddr,"offset:",offset
					func.translateAddresses(offset)
				
		line = objdumpFile.readline()

	objdumpFile.close()

# Gets the file id for a file name. The path of the file is ignored.
def getIdForFileName(name):
	nameWithoutPath = name.split("/")[-1]	
	for (key, value) in files.iteritems():
		valueWithoutPath = value.split("/")[-1]	
		if valueWithoutPath == nameWithoutPath:
			return key
	return None	

def newSkipBlock():
	return ()

class SkipBlock:
	def __init__(self):
		self.skipFrom = 0
		self.skipTo = 0

	def __repr__(self):
		return "<skipBlock skipFrom:%s skipTo:%s>\n" % (hex(self.skipFrom), hex(self.skipTo))

def buildSkipList():
	tscribeId = getIdForFileName("tscribe.c")
	skipBlock = None
	skipList = []
	for (funcName, func) in itertools.ifilterfalse(lambda (name, x): name.startswith("tscribe_"), functions.iteritems()):
		lastLocSkiped = False
		for loc in func.locations:
			# ignore empty locations
			if loc.isEmpty():
				pass
			elif loc.fileId == tscribeId:
				if not lastLocSkiped:
					skipBlock = SkipBlock()
					skipBlock.skipFrom = loc.firstAddr()
					#print "from",skipBlock.skipFrom
				lastLocSkiped = True
			else:
				if lastLocSkiped:
					skipBlock.skipTo = loc.firstAddr()
					skipList.append(skipBlock)
					#print "to",skipBlock.skipTo
				lastLocSkiped = False
	return skipList

def skipListToFile(skipList):
	for skip in skipList:	
		binMapFile.write("skip %s %s\n" % (hex(skip.skipFrom), hex(skip.skipTo)))

encounteredReads = {}

def regReadsToFile():
	regReadId = getIdForFileName("read_register")
	for (funcName, func) in functions.iteritems():
		for loc in func.locations:
			if loc.fileId == regReadId:
				readId = loc.lineNum - 1000
				# bug with CIL code causes some read locs to get marked twice
				if (not (readId in encounteredReads)):
					encounteredReads[readId] = 1
					#print "read register", readId, ":",
					#loc.printAddresses()
					binMapFile.write("read %s %d\n" % (hex(loc.firstAddr()), readId))

def funcsToFile():
	for (funcName, func) in functions.iteritems():
		binMapFile.write("func %s %s %s\n" % (hex(func.firstAddr()), hex(func.lastAddr()), funcName))

def fileNamesToFile():
	for (fileId, fileName) in files.iteritems():
		binMapFile.write("file %s %d\n" % (fileName, fileId))

def linesToFile():
	for (funcName, func) in functions.iteritems():
		for loc in func.locations:
			firstAddr = loc.firstAddr()
			if firstAddr != None:
				binMapFile.write("line %s %d %d\n" % (hex(firstAddr), loc.fileId, loc.lineNum))

def globalVarsToFile():
	for (varName, addr) in globalVars.iteritems():
		binMapFile.write("var %s %s\n" % (hex(addr), varName))

#for (name, func) in functions.items():
#	print name, hex(func.firstAddr())

parseAs()
parseLd()
parseObjdump()
regReadsToFile()
funcsToFile()
fileNamesToFile()
linesToFile()
globalVarsToFile()

