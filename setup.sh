#!/bin/bash

export TSCRIBE=
export CIL_HOME=
export CIL_MACHINE=
export HOST_OS=
export MOTEDEV=

TSCRIBE=`pwd`
CIL_HOME=$TSCRIBE/transpiler
unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
	HOST_OS="x86_LINUX"
elif [[ "$unamestr" == 'Darwin' ]]; then
	HOST_OS="x86_DARWIN"
fi
# WARNING: something wrong with following definition, tinyos and contiki code does not execute correctly on msp430
CIL_MACHINE="bool=1,2 short=2,2 int=2,2 long=4,2 long_long=8,2 float=4,2 double=4,2 long_double=4,2 void=1 pointer=2,2 enum=2,2 fun=1,2 alignof_string=1 alignof_enum=1 max_alignment=2 size_t=unsigned_int wchar_t=unsigned_int char_signed=true big_endian=false const_string_literals=true __thread_is_keyword=false __builtin_va_list=true underscore_name=true"
MOTEDEV=/dev/tty.SLAB_USBtoUART

export TSCRIBE
export CIL_HOME
export CIL_MACHINE
export HOST_OS
export MOTEDEV

export PATH=$TSCRIBE/transpiler/bin:$PATH
export PATH=$TSCRIBE/targets/common/scripts:$PATH
export PATH=$TSCRIBE/targets/tinyos/make:$PATH
export PATH=$TSCRIBE/targets/noos/make:$PATH
export PATH=$TSCRIBE/targets/contiki/make:$PATH
export PATH=$TSCRIBE/tools:$PATH

export CLASSPATH=$TSCRIBE/tools:\
$TSCRIBE/emulator/mspsim/build:\
$TSCRIBE/emulator/mspsim/lib/jcommon-1.0.14.jar:\
$TSCRIBE/emulator/mspsim/lib/jfreechart-1.0.11.jar:\
$TSCRIBE/emulator/mspsim/lib/jipv6.jar:\
$TSCRIBE/emulator/mspsim/lib/json-simple-1.1.1.jar:\
$CLASSPATH

