*****************************************************************
Build Instructions for GeoDa.  Current as of GeoDa 1.8.x
*****************************************************************

Overview: We assume the build machine hosts a recently-installed
clean OS.  This build guide contains notes on setting up the compile
environment, obtaining the GeoDa source files and dependent libraries,
compiling libraries and GeoDa, and finally packaging the program
for distribution and installation.

****************************************************
 Building GeoDa for 64-bit OSX 10.6.8 
****************************************************

NOTE: This is just basic placeholder for now!  Not currently complete.

Build machine assumptions:
- clean OSX 10.6.8 installation with all OS updates.
- note the current build script will likely only work on OS 10.6.8
  and not later versions of OSX.  It is very difficult to target
  previous versions of OSX, so we have chosen to build on the minimum
  version supported.

1. Install XCode 3.2
 - this will result in Xcode and all c++ compilers as well as svn
 being installed.

2. Use Git to check out GeoDa trunk:
 - From user's home directory: ~/
 - git clone https://github.com/GeoDaCenter/geoda.git
 
3. cd to ~/trunk/BuildTools/macosx

4. run ./build.sh 8 to download and build GeoDa and everything it
depends upon.
 - The number 8 should be changed to however many individual cores the
 build machine has available for parallel compilation.

5. Package GeoDa for distribution / installation.

****************************************************
 Building GeoDa for 64-bit OSX 10.8 or later
****************************************************

1. Install XCode 

2. Use Git to check out GeoDa trunk:
 - From user's home directory: ~/
 - git clone https://github.com/GeoDaCenter/geoda.git
 
3. cd to ~/trunk/BuildTools/macosx

4. run ./build-express.sh to download prebuild libraries for GeoDa

5. start GeoDa project using the GeoDa.xcodeproj with Xcode



****************************************************
 Building GeoDa plugins for 64-bit OSX 
****************************************************

1. Download Oracle Instant Client Package - Basic & Oracle Instant Client Package - SDK

2. Unzip SDK, then unzip "Basic", which will be copied to lib/

3. Create the appropriate libclntsh.dylib link for the version of Instant Client. For example:
    ```
    cd ~/instantclient_11_2
    ln -s libclntsh.dylib.11.1 libclntsh.dylib
    ```
4. Update ORACLE_HOME
    ```
    export ORACLE_HOME=~/instantclient_11_2
    ```

5. Build Oracle plugin
    ```
    cd GDAL_HOME/ogr/ogrsf_frmts/oci
    make plugin
    install_name_tool -change '~/geoda_trunk/BuildTools/macosx/libraries/lib/libgdal.20.dylib' '@executable_path/../Resources/plugins/libgdal.20.dylib' ogr_OCI.so
    mv ogr_OCI.so ~/geoda_trunk/BuildTools/macosx/plugins
    ```

****************************************************
 Building GeoDa plugins for 64-bit OSX 
****************************************************

1. Download File Geodatabase API 1.3 version for Mac 64-bit 

2. Unzip

3. Update FGDB_HOME
    ```
    export FGDB_HOME=~/FileGDB_API
    ```

5. Build FileGDB plugin
    ```
    cd GDAL_HOME/ogr/ogrsf_frmts/filegdb
    make plugin
    nstall_name_tool -change '/Users/xun/geoda_trunk/BuildTools/macosx/libraries/lib/libgdal.20.dylib' '@executable_path/../Resources/plugins/libgdal.20.dylib' ogr_FileGDB.so
    mv ogr_FileGDB.so ~/geoda_trunk/BuildTools/macosx/plugins/
    ```
