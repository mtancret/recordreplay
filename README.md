recordreplay
==========

Record and replay for TelosB motes, based on:

M. Tancreti, V. Sundaram, S. Bagchi, and P. Eugster.
TARDIS: Software-Only System-Level Record and Replay in Wireless Sensor Networks.
To appear in the 14th International Conference on Information Processing in Sensor Networks (IPSN).
ACM, 2015.

Prerequisites
----------

TinyOS 2.x  
Copy tinyos.sh from tools into main tinyos directory and run
> source tinyos.sh

Usage
----------

Set up paths using the setup.sh script.
> source setup.sh

Compile an example TinyOS application (requires TinyOS to be installed and configured).
> cd targets/tinyos/examples/Blink  
> make-tinyos  
It is normal to see "collect2: ld returned 1 exit status"
Check the compile log file build/telosb/make\_tscribe.out for errors.

Program the mote, MOTEDEV needs to be set to the mote's device location.
> export MOTEDEV=/dev/tty.usbserial-XBTLWQL1  
> install-tinyos  

To enter command mode press and release the reset button while holding down the user button.
Now press and release the user button again to dump the contents of flash to the serial port.
To capture the contents of flash run collect.py before pressing the user button.
> collect.py  
Press and release the user button two more times to clear the flash.
Exit command mode at any time by pressing reset.

Output of collect.py:  
build/log-gen.dat log of generic register trace  
build/log-state.dat log of state register trace  
build/log-int.dat log of interrupt trace  
build/log-err.dat log of error trace  

Organization
----------
README.md - Main instructions.  
emulator - Modifed version of mspsim.  
targets - Code that is compiled into the target OS.  
> common - Code that is used across all OSs.  
> tinyos - TinyOS target specific code.  
> > examples - Example TinyOS apps used for testing.  
> > make - Scripts used to compile applications.  
tools - Collection of usefull scripts for collecting and analysing data.  
transpiler - Coverts C code into record instrumented code.  
setup.sh - Script to set up environment.  
