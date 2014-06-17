@echo OFF

REM call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
REM call "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
REM call "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
if %PROCESSOR_ARCHITECTURE% == x86 (
  call "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
  REM call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"  
) else if %PROCESSOR_ARCHITECTURE% == AMD64 (
  if NOT EXIST "C:\Program Files (x86)\Microsoft Visual Studio 10.0\Microsoft Visual C++ 2010 Express - ENU\License.txt" (
	call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" amd64
	echo Looks like you are using Visual Studio 2010 Pro
  ) else (
	call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64
	echo Looks like you are using Visual Studio 2010 Express
  )
  REM Windows SDK for Windows 7 must be installed before the above command will work
  REM Please follow steps here to fully patch SDK 7.1 for 64-bit machines:
  REM   http://forum.celestialmatters.org/viewtopic.php?t=404
  REM
  REM call "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\vcvarsall.bat" x64
  REM The above amd64 environment option does not work with the default VS 2010 Express installation.
)

echo.
echo #####################################################
echo #
echo #  OGR Plugin build script for Windows 7, 32/64-bit
echo #  
echo #  Note: This script assumes that GDAL/OGR has
echo #        already been built.
echo #  
echo #  PROCESSOR_ARCHITECTURE: %PROCESSOR_ARCHITECTURE%

set GDA_BUILD=unknown
if %PROCESSOR_ARCHITECTURE% == x86 (
  echo #  32-bit Windows detected
  set GDA_BUILD=BUILD_32
) else if %PROCESSOR_ARCHITECTURE% == AMD64 (
  echo #  64-bit Windows detected
  set GDA_BUILD=BUILD_64
) else (
  echo #  Could not determine processor architecture.
  echo #  Exiting...
)

echo #
echo #####################################################
echo.
if %GDA_BUILD% == unknown goto PLUGIN_BUILD_END

set BUILD_HOME=%CD%
echo BUILD_HOME: %BUILD_HOME%
set GEODA_HOME=%BUILD_HOME%\..\..
echo GEODA_HOME: %GEODA_HOME%
set DOWNLOAD_HOME=%BUILD_HOME%\temp
echo DOWNLOAD_HOME: %DOWNLOAD_HOME%
set BUILD_DEP=%BUILD_HOME%\dep
echo BUILD_DEP: %BUILD_DEP%
set UNZIP_EXE=%BUILD_DEP%\7za.exe x
echo UNZIP_EXE: %UNZIP_EXE%
set CURL_EXE=%BUILD_DEP%\curl.exe
echo CURL_EXE: %CURL_EXE%
set MSBUILD_EXE=msbuild
echo MSBUILD_EXE: %MSBUILD_EXE%
if %GDA_BUILD% == BUILD_32 (
  set LIBRARY_HOME=C:\OSGeo4W
  set LIB_HM_LIB = lib
)
if %GDA_BUILD% == BUILD_64 (
  set LIBRARY_HOME=C:\OSGeo4W
  set LIB_HM_LIB = lib
)
echo LIBRARY_HOME: %LIBRARY_HOME%

REM rmdir %LIBRARY_HOME% /s /q

IF NOT EXIST %LIBRARY_HOME% (
    md %LIBRARY_HOME%
    md %LIBRARY_HOME%\%LIB_HM_LIB%
    md %LIBRARY_HOME%\include
    md %LIBRARY_HOME%\plugins
)

IF NOT EXIST %DOWNLOAD_HOME% (
    md %DOWNLOAD_HOME%
)

REM #Create a empty unix header file, just for ref
echo. 2> %LIBRARY_HOME%\include\unistd.h


set LIB_NAME=gdal-1.9.2
echo LIB_NAME: %LIB_NAME%
echo.

echo #####################################################
echo #   build ESRI FileGDB plugin
echo #####################################################
if %GDA_BUILD% == BUILD_32 (
copy /Y %BUILD_HOME%\dep\%LIB_NAME%\nmake.opt.withplugins %DOWNLOAD_HOME%\%LIB_NAME%\nmake.opt
) else (
copy /Y %BUILD_HOME%\dep\%LIB_NAME%\nmake64.opt.withplugins %DOWNLOAD_HOME%\%LIB_NAME%\nmake.opt
)
cd %DOWNLOAD_HOME%\%LIB_NAME%\ogr\ogrsf_frmts\filegdb
nmake -f makefile.vc plugin
copy /Y %LIB_NAME%\ogr\ogrsf_frmts\filegd\ogr_FileGDB.dll %LIBRARY_HOME%\plugins 

echo #####################################################
echo #   build Oracle plugin
echo #####################################################
cd %DOWNLOAD_HOME%\%LIB_NAME%\ogr\ogrsf_frmts\oci
nmake -f makefile.vc ogr_OCI.dll
copy /Y %LIB_NAME%\ogr\ogrsf_frmts\oci\ogr_OCI.dll %LIBRARY_HOME%\plugins 

echo #####################################################
echo #   build ESRI Arc SDE
echo #####################################################
cd %DOWNLOAD_HOME%\%LIB_NAME%\ogr\ogrsf_frmts\sde
nmake -f makefile.vc ogr_SDE.dll
copy /Y %LIB_NAME%\ogr\ogrsf_frmts\sde\ogr_SDE.dll %LIBRARY_HOME%\plugins 

cd %BUILD_HOME%
echo.
echo Finished GDAL/OGR Plugin build

:PLUGIN_BUILD_END
