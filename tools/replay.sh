#!/bin/bash

BUILD_DIR=$1
PLATFORM_DIR=$2

# Using -Xint to disable JIT, had seg fault in libjvm.dylib, see below
java -Xint se.sics.mspsim.platform.sky.SkyNode \
	-intLog=$BUILD_DIR/log-int.dat \
	-stateLog=$BUILD_DIR/log-state.dat \
	-genLog=$BUILD_DIR/log-gen.dat \
	-valuesLog=$BUILD_DIR/log-values.dat \
	-countsLog=$BUILD_DIR/log-counts.dat \
	-codeMap=$PLATFORM_DIR/code-map.out \
	-addrMap=$PLATFORM_DIR/address-map.out \
	$PLATFORM_DIR/main-cil.ihex

# TODO: What is wrong with JRE 7.0_15-b03 runing on OSX 10.8.5?
#
# A fatal error has been detected by the Java Runtime Environment:
#
#  SIGSEGV (0xb) at pc=0x000000010b6cbbd3, pid=50802, tid=20739
#
# JRE version: 7.0_15-b03
# Java VM: Java HotSpot(TM) 64-Bit Server VM (23.7-b01 mixed mode bsd-amd64 compressed oops)
# Problematic frame:
# V  [libjvm.dylib+0x3d9bd3]
#
# Failed to write core dump. Core dumps have been disabled. To enable core dumping, try "ulimit -c unlimited" before starting Java again
#
# If you would like to submit a bug report, please visit:
#   http://bugreport.sun.com/bugreport/crash.jsp
#

