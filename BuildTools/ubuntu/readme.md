*****************************************************************
Build Instructions for GeoDa.  Current as of GeoDa 1.8.x 
*****************************************************************

Overview: We assume the build machine hosts a recently-installed
clean OS.  This build guide contains notes on setting up the compile
environment, obtaining the GeoDa source files and dependent libraries,
compiling libraries and GeoDa, and finally packaging the program
for distribution and installation.

*******************************************************
Building GeoDa for 32-bit Ubuntu 12.04 or later 
*******************************************************

NOTE: This is just basic placeholder for now!  Not currently complete.

Build machine assumptions:
- clean Ubuntu 32-bit installation with all OS updates

1. Install C++ developer tools along with command-line subversion

2. Use Git to check out GeoDa trunk:
 - From user's home directory: ~/
 - git clone https://github.com/GeoDaCenter/geoda.git
 
3. cd to ~/geoda/BuildTools/ubuntu

4. run ./build.sh to download and build GeoDa and everything it depends upon
--you need to manually build ogr plugins (will do later)

5. Package GeoDa for distribution / installation.
-- run create_deb.sh, you will find dep package geoda*.deb in current directory 


*******************************************************
Building GeoDa for 64-bit Ubuntu 12.04 or later
*******************************************************

NOTE: This is just basic placeholder for now!  Not currently complete.

Build machine assumptions:
- clean Ubuntu 64-bit installation with all OS updates

1. Install C++ developer tools along with command-line subversion

2. Use Git to check out GeoDa trunk:
 - From user's home directory: ~/
 - git clone https://github.com/GeoDaCenter/geoda.git
 
3. cd to ~/geoda/BuildTools/ubuntu

4. run ./build_**version**.sh to download and build GeoDa and everything it depends upon
--you need to manually build ogr plugins.
e.g. for ubuntu bionic versoin run ./build_bionic.sh
5. To run the GeoDa cd to "build" and run **./run.sh**

5. Package GeoDa for distribution / installation.
-- run create_deb.sh, you will find dep package geoda*.deb in current directory 

