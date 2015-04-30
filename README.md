recordreplay
==========

This repository contains record and replay for TelosB motes, based on the following work.

M. Tancreti, V. Sundaram, S. Bagchi, and P. Eugster.
TARDIS: Software-Only System-Level Record and Replay in Wireless Sensor Networks.
To appear in the 14th International Conference on Information Processing in Sensor Networks (IPSN).
ACM, 2015.

License
----------

The files in this repository are provided as is, for non-commercial purposes only.

Prerequisites
----------

Linux x86 or Mac OS X

TinyOS 2.x  

Make sure that TinyOS paths are configured correctly, for example, copy tinyos.sh from tools into main tinyos directory and execute

> source tinyos.sh

Usage
----------

Clone repository.

> git clone git@github.com:mtancret/recordreplay.git  
> cd recordreplay  

Configure paths using the setup.sh script (you may want to place this in your shell startup files).

> source setup.sh

Compile an example TinyOS application (requires TinyOS to be installed and configured).

> cd targets/tinyos/examples/Blink  
> make-tinyos  

It is normal to see "collect2: ld returned 1 exit status"
Check the compile log file build/telosb/log.out for errors.

Program the mote, MOTEDEV needs to be set to your mote's device location.

> export MOTEDEV=/dev/ttyUSB0  
> install-tinyos  

First make sure the contents of flash are clear by using the following procedure.
To enter command mode press and release the reset button while holding down the user button.
Now press and release the user button a second time to dump the contents of flash to the serial port.
After the light turns green, press the user button two more times to erase the contents of flash.
The light will turn red during flash erase and then green when flash is clear.
Press reset to restart the mote in record mode.

Let the mote record for 30 seconds.
Then enter command mode again by pressing and releasing the reset button while holding down the user button. Start the collection script.

> collect.py

Press and release the user button a second time to dump the contents of flash to the serial port.

The output of collect.py should be the following.  
build/log-gen.dat log of generic register trace  
build/log-state.dat log of state register trace  
build/log-int.dat log of interrupt trace  
build/log-err.dat log of error trace  

The replay the log run the replay.sh script.

> replay.sh

When the emulator opens press run.

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

