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

for /f "tokens=4 delims=;= " %%P in ('findstr /c:"version_major" ..\..\version.h') do set VER_MAJOR=%%P
for /f "tokens=4 delims=;= " %%P in ('findstr /c:"version_minor" ..\..\version.h') do set VER_MINOR=%%P
for /f "tokens=4 delims=;= " %%P in ('findstr /c:"version_build" ..\..\version.h') do set VER_BUILD=%%P
set GDA_VERSION=%VER_MAJOR%.%VER_MINOR%.%VER_BUILD%

echo.
echo #####################################################
echo #
echo #  Build script for Windows 7, 32-bit and 64-bit
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
echo #  GeoDa version %GDA_VERSION%
echo #
echo #####################################################
echo.

set CL=/MP

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
set CURL_EXE=%BUILD_DEP%\curl.exe -k
echo CURL_EXE: %CURL_EXE%
set MSBUILD_EXE= msbuild
echo MSBUILD_EXE: %MSBUILD_EXE%
if %GDA_BUILD% == BUILD_32 (
  set LIBRARY_HOME=C:\OSGeo4W
  set LIB_HM_LIB=lib
)
if %GDA_BUILD% == BUILD_64 (
  set LIBRARY_HOME=C:\OSGeo4W
  set LIB_HM_LIB=lib
)
echo LIBRARY_HOME: %LIBRARY_HOME%
echo LIB_HM_LIB: %LIB_HM_LIB%

REM rmdir %LIBRARY_HOME% /s /q

IF NOT EXIST %LIBRARY_HOME% md %LIBRARY_HOME%
IF NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB% md %LIBRARY_HOME%\%LIB_HM_LIB%
IF NOT EXIST %LIBRARY_HOME%\include md %LIBRARY_HOME%\include
IF NOT EXIST %LIBRARY_HOME%\plugins md %LIBRARY_HOME%\plugins

IF NOT EXIST %DOWNLOAD_HOME% md %DOWNLOAD_HOME%

REM #Create a empty unix header file, just for ref
echo. 2> %LIBRARY_HOME%\include\unistd.h

:TO_CURL_BUILD
echo.
echo #####################################################
echo #   build cURL (by default /MD) 
echo #####################################################
echo.
set LIB_NAME=curl-7.46.0
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/curl-7.46.0.zip"

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl_a.lib set ALL_EXIST=false

if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_CURL_BUILD
)

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

cd %DOWNLOAD_HOME%\%LIB_NAME%\winbuild
if %GDA_BUILD% == BUILD_32 (
  rmdir %DOWNLOAD_HOME%\%LIB_NAME%\builds /s /q
  nmake -f Makefile.vc mode=static CONFIG_NAME_LIB=curlib
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\builds\curlib\lib\libcurl_a.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl_a.lib
  
  rmdir %DOWNLOAD_HOME%\%LIB_NAME%\builds /s /q
  nmake -f Makefile.vc mode=dll CONFIG_NAME_LIB=curlib

  xcopy /E /Y %DOWNLOAD_HOME%\%LIB_NAME%\builds\curlib\include %LIBRARY_HOME%\include
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\builds\curlib\lib\libcurl.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.lib
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\builds\curlib\bin\libcurl.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.dll


) else (
  rmdir %DOWNLOAD_HOME%\%LIB_NAME%\builds /s /q
  nmake -f Makefile.vc mode=static CONFIG_NAME_LIB=curlib
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\builds\curlib\lib\libcurl_a.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl_a.lib

  rmdir %DOWNLOAD_HOME%\%LIB_NAME%\builds /s /q
  nmake -f Makefile.vc mode=dll CONFIG_NAME_LIB=curlib
  
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\builds\curlib\bin\libcurl.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.dll
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\builds\curlib\lib\libcurl.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.lib
  xcopy /E /Y %DOWNLOAD_HOME%\%LIB_NAME%\builds\curlib\include %LIBRARY_HOME%\include

)
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\libcurl_a.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_CURL_BUILD


:TO_ICONV_BUILD
echo.
echo #####################################################
echo #   build libiconv
echo #####################################################
echo.
set LIB_NAME=libiconv-1.14
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/libiconv-1.14.tar.gz"

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\iconv.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_ICONV_BUILD
)

cd %DOWNLOAD_HOME%

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.tar.gz %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.tar.gz
    %UNZIP_EXE% %LIB_NAME%.tar.gz
	%UNZIP_EXE% %LIB_NAME%.tar
)

xcopy /E /Y %BUILD_DEP%\%LIB_NAME% %DOWNLOAD_HOME%\%LIB_NAME%
cd %DOWNLOAD_HOME%\%LIB_NAME%

if %GDA_BUILD% == BUILD_32 (
  %MSBUILD_EXE% libiconv.sln /property:Configuration="Release" /p:Platform="Win32"
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\iconv.h %LIBRARY_HOME%\include\iconv.h
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Release_Win32\libiconv.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libiconv.dll
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Release_Win32\libiconv.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libiconv.lib
  
  %MSBUILD_EXE% libiconv.sln /property:Configuration="Release_static" /p:Platform="Win32"
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Release_static_Win32\libiconv.lib %LIBRARY_HOME%\%LIB_HM_LIB%\iconv.lib
  
) else (

  %MSBUILD_EXE% libiconv.sln /property:Configuration="Release" /p:Platform="x64"
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\iconv.h %LIBRARY_HOME%\include
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Release_x64\libiconv.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libiconv.dll
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Release_x64\libiconv.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libiconv.lib

  %MSBUILD_EXE% libiconv.sln /property:Configuration="Release_static" /p:Platform="x64"
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Release_static_x64\libiconv.lib %LIBRARY_HOME%\%LIB_HM_LIB%\iconv.lib
)
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\iconv.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_ICONV_BUILD


:TO_XERCES_BUILD
echo.
echo #####################################################
echo #   build Xerces
echo #####################################################
echo.
set LIB_NAME=xerces-c-3.1.1
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/xerces-c-3.1.1.zip"
set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\xerces-c_static_3.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_XERCES_BUILD
)

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

xcopy /E /Y %BUILD_DEP%\%LIB_NAME% %DOWNLOAD_HOME%\%LIB_NAME%

if %GDA_BUILD% == BUILD_32 (
  %MSBUILD_EXE% %LIB_NAME%\projects\Win32\VC10\xerces-all\xerces-all.sln /t:XercesLib /property:Configuration="Release"

  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\*.h %LIBRARY_HOME%\include
  md %LIBRARY_HOME%\include\xercesc
  xcopy /E /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\xercesc %LIBRARY_HOME%\include\xercesc

  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Build\Win32\VC10\Release\xerces-c_3.lib %LIBRARY_HOME%\%LIB_HM_LIB%\xerces-c_3.lib
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Build\Win32\VC10\Release\xerces-c_3_1.dll %LIBRARY_HOME%\%LIB_HM_LIB%\xerces-c_3_1.dll

  %MSBUILD_EXE% %LIB_NAME%\projects\Win32\VC10\xerces-all\xerces-all.sln /t:XercesLib /property:Configuration="Static Release"
  copy /Y "%DOWNLOAD_HOME%\%LIB_NAME%\Build\Win32\VC10\Static Release\"xerces-c_static_3.lib %LIBRARY_HOME%\%LIB_HM_LIB%\xerces-c_static_3.lib

) else (

  %MSBUILD_EXE% %LIB_NAME%\projects\Win32\VC10\xerces-all\xerces-all.sln /t:XercesLib /property:Configuration="Release" /p:Platform="x64"

  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\*.h %LIBRARY_HOME%\include
  md %LIBRARY_HOME%\include\xercesc
  xcopy /E /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\xercesc %LIBRARY_HOME%\include\xercesc

  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Build\Win64\VC10\Release\xerces-c_3.lib %LIBRARY_HOME%\%LIB_HM_LIB%\xerces-c_3.lib
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\Build\Win64\VC10\Release\xerces-c_3_1.dll %LIBRARY_HOME%\%LIB_HM_LIB%\xerces-c_3_1.dll

  %MSBUILD_EXE% %LIB_NAME%\projects\Win32\VC10\xerces-all\xerces-all.sln /t:XercesLib /property:Configuration="Static Release" /p:Platform="x64"
  copy /Y "%DOWNLOAD_HOME%\%LIB_NAME%\Build\Win64\VC10\Static Release\"xerces-c_static_3.lib %LIBRARY_HOME%\%LIB_HM_LIB%\xerces-c_static_3.lib
)
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\xerces-c_static_3.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_XERCES_BUILD

:TO_GEOS_BUILD
echo.
echo #####################################################
echo #   build GEOS
echo #####################################################
echo.
set LIB_NAME=geos-3.3.8
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/geos-3.3.8.tar.bz2"
set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\geos.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\geos.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\geos_c.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\geos_c_i.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\geos_i.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_GEOS_BUILD
)

cd %DOWNLOAD_HOME%

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.tar.bz2 %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.tar.bz2
    %UNZIP_EXE% %LIB_NAME%.tar.bz2
	%UNZIP_EXE% %LIB_NAME%.tar
)

cd %LIB_NAME%
call autogen.bat

REM patch nmake.opt to support nmake with version string 10.00.40219.01
copy /Y %BUILD_HOME%\dep\%LIB_NAME%\nmake.opt %DOWNLOAD_HOME%\%LIB_NAME%\nmake.opt
nmake -f makefile.vc MSVC_VER=1600 BUILD_DEBUG=NO

copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\capi\*.h %LIBRARY_HOME%\include
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\include\*.h %LIBRARY_HOME%\include
md %LIBRARY_HOME%\include\geos
xcopy /E /Y %DOWNLOAD_HOME%\%LIB_NAME%\include\geos %LIBRARY_HOME%\include\geos

copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\geos.dll %LIBRARY_HOME%\%LIB_HM_LIB%\geos.dll
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\geos.lib %LIBRARY_HOME%\%LIB_HM_LIB%\geos.lib
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\geos_c.dll %LIBRARY_HOME%\%LIB_HM_LIB%\geos_c.dll
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\geos_c_i.lib %LIBRARY_HOME%\%LIB_HM_LIB%\geos_c_i.lib
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\geos_i.lib %LIBRARY_HOME%\%LIB_HM_LIB%\geos_i.lib

set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\geos.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\geos.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\geos_c.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\geos_c_i.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\geos_i.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_GEOS_BUILD


:TO_PROJ4_BUILD
echo.
echo #####################################################
echo #   build PROJ4 (todo , MD,MT nmake.opt)
echo #####################################################
echo.
set LIB_NAME=proj-4.8.0
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/proj-4.8.0.zip"

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\proj.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\proj.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\proj_i.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_PROJ4_BUILD
)

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

cd %LIB_NAME%
nmake -f makefile.vc

set PROJ_INCLUDE=%DOWNLOAD_HOME%\%LIB_NAME%\src

copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\*.h %LIBRARY_HOME%\include
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\*.lib %LIBRARY_HOME%\%LIB_HM_LIB%
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\*.dll %LIBRARY_HOME%\%LIB_HM_LIB%

set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\proj.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\proj.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\proj_i.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_PROJ4_BUILD


:TO_FREEXL_BUILD
echo.
echo #####################################################
echo #   build freeXL
echo #####################################################
echo.
set LIB_NAME=freexl-1.0.0e
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/freexl-1.0.0e.zip"

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\freexl.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\freexl.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\freexl_i.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_FREEXL_BUILD
)

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

cd %LIB_NAME%

copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\headers\*.h %LIBRARY_HOME%\include

nmake -f makefile.vc OPTFLAGS="/MD /DDLL_EXPORT -DUSING_STATIC_LIBICONV -I%LIBRARY_HOME%\include" OPTLKFLAG="%LIBRARY_HOME%\%LIB_HM_LIB%\iconv.lib"

copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\freexl_i.lib %LIBRARY_HOME%\%LIB_HM_LIB%\freexl_i.lib
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\freexl.lib %LIBRARY_HOME%\%LIB_HM_LIB%\freexl.lib
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\freexl.dll %LIBRARY_HOME%\%LIB_HM_LIB%\freexl.dll

set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\freexl.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\freexl.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\freexl_i.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_FREEXL_BUILD


:TO_SQLITE_BUILD
echo.
echo #####################################################
echo #   build SQLite3
echo #####################################################
echo.
set LIB_NAME=sqlite-amalgamation-3071700
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/sqlite-amalgamation-3071700.zip"

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite3_i.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\sqlited.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_SQLITE_BUILD
)

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

xcopy /Y /E %BUILD_DEP%\%LIB_NAME% %DOWNLOAD_HOME%\%LIB_NAME%
cd %DOWNLOAD_HOME%\%LIB_NAME%\sqlite

if %GDA_BUILD% == BUILD_32 (
  %MSBUILD_EXE% sqlite.sln /property:Configuration="Release_dll"
 
  REM lib /def:sqlite\sqlite3.def

  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\sqlite\Release_dll\sqlite.lib %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite3_i.lib
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\sqlite\Release_dll\sqlite.dll %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.dll

  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\*.h %LIBRARY_HOME%\include

  %MSBUILD_EXE% sqlite.sln /property:Configuration="Release"
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\sqlite\Release\sqlite.lib %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.lib
  
  %MSBUILD_EXE% sqlite.sln /property:Configuration="Debug"
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\sqlite\Debug\sqlite.lib %LIBRARY_HOME%\%LIB_HM_LIB%\sqlited.lib
  
) else (
  %MSBUILD_EXE% sqlite.sln /property:Configuration="Release_dll" /p:Platform="x64"

  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\sqlite\x64\Release_dll\sqlite.lib %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite3_i.lib
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\sqlite\x64\Release_dll\sqlite.dll %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.dll

  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\*.h %LIBRARY_HOME%\include

  %MSBUILD_EXE% sqlite.sln /property:Configuration="Release" /p:Platform="x64"
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\sqlite\x64\Release\sqlite.lib %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.lib

  %MSBUILD_EXE% sqlite.sln /property:Configuration="Debug" /p:Platform="x64"
  copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\sqlite\x64\Debug\sqlite.lib %LIBRARY_HOME%\%LIB_HM_LIB%\sqlited.lib
)
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\sqlite3_i.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\sqlited.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_SQLITE_BUILD


:TO_SPATIALITE_BUILD
echo.
echo #####################################################
echo #   build SpatiaLite
echo #####################################################
echo.
set LIB_NAME=libspatialite-4.0.0
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/libspatialite-4.0.0.zip"

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\spatialite.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\spatialite.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\spatialite_i.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_SPATIALITE_BUILD
)

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

if %GDA_BUILD% == BUILD_32 (
  copy /Y %BUILD_DEP%\%LIB_NAME%\makefile.vc %DOWNLOAD_HOME%\%LIB_NAME%\makefile.vc
) else (
  copy /Y %BUILD_DEP%\%LIB_NAME%\makefile64.vc %DOWNLOAD_HOME%\%LIB_NAME%\makefile.vc
)

cd %DOWNLOAD_HOME%\%LIB_NAME%
nmake -f makefile.vc 

md %LIBRARY_HOME%\include\spatialite
xcopy /Y /E %DOWNLOAD_HOME%\%LIB_NAME%\src\headers\spatialite  %LIBRARY_HOME%\include\spatialite
copy /Y %LIBRARY_HOME%\include\sqlite3.h %LIBRARY_HOME%\include\spatialite\sqlite3.h
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\src\headers\*.h %LIBRARY_HOME%\include
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\spatialite.dll %LIBRARY_HOME%\%LIB_HM_LIB%\spatialite.dll
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\spatialite_i.lib %LIBRARY_HOME%\%LIB_HM_LIB%\spatialite_i.lib
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\spatialite.lib %LIBRARY_HOME%\%LIB_HM_LIB%\spatialite.lib

set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\spatialite.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\spatialite.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\spatialite_i.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_SPATIALITE_BUILD


:TO_POSTGRESQL_BUILD
echo.
echo #####################################################
echo #   build PostgreSQL
echo #####################################################
echo.
set LIB_NAME=postgresql-9.2.4
if %GDA_BUILD% == BUILD_32 (
  set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/postgresql-9.2.4-1-windows-binaries.zip"
) else (
  set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/postgresql-9.2.4-1-windows-x64-binaries.zip"
)

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\libpq.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\libpq.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_POSTGRESQL_BUILD
)

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%\pgsql (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

if %GDA_BUILD% == BUILD_32 (
  copy /Y %DOWNLOAD_HOME%\pgsql\bin\libintl.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libintl.dll
  copy /Y %DOWNLOAD_HOME%\pgsql\bin\libeay32.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libeay32.dll
  copy /Y %DOWNLOAD_HOME%\pgsql\bin\ssleay32.dll %LIBRARY_HOME%\%LIB_HM_LIB%\ssleay32.dll
  copy /Y %DOWNLOAD_HOME%\pgsql\lib\libpq.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libpq.lib
  copy /Y %DOWNLOAD_HOME%\pgsql\lib\libpq.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libpq.dll
  xcopy /Y /E %DOWNLOAD_HOME%\pgsql\include %LIBRARY_HOME%\include
) else (

  copy /Y %DOWNLOAD_HOME%\pgsql\bin\libintl-8.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libintl-8.dll
  copy /Y %DOWNLOAD_HOME%\pgsql\bin\libeay32.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libeay32.dll
  copy /Y %DOWNLOAD_HOME%\pgsql\bin\ssleay32.dll %LIBRARY_HOME%\%LIB_HM_LIB%\ssleay32.dll
  copy /Y %DOWNLOAD_HOME%\pgsql\lib\libpq.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libpq.lib
  copy /Y %DOWNLOAD_HOME%\pgsql\lib\libpq.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libpq.dll
  xcopy /Y /E %DOWNLOAD_HOME%\pgsql\include %LIBRARY_HOME%\include
)
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\libpq.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\libpq.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_POSTGRESQL_BUILD


:TO_MYSQL_BUILD
echo.
echo #####################################################
echo #  build MySQL 
echo #####################################################
echo.
if %GDA_BUILD% == BUILD_32 (
  set LIB_NAME=mysql-5.6.16-win32
  set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/mysql-5.6.14-win32.zip"
) else (
  set LIB_NAME=mysql-5.6.16-winx64
  set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/mysql-5.6.14-winx64.zip"
)

REM "The downloaded DLLs are /MT builds, which can't be used. I think build "
REM "MysQL on windows is pretty complex, so I just did that once, and copy the DLLs"
REM "to plugins directory for our usage."
set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\libmysql.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\mysqlclient.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_MYSQL_BUILD
)

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%/%LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

if %GDA_BUILD% == BUILD_32 (
  xcopy /Y /E %BUILD_HOME%\dep\mysql\my_default.h %DOWNLOAD_HOME%\%LIB_NAME%\include
  copy /Y  %BUILD_HOME%\plugins\mysqlclient.lib %LIBRARY_HOME%\%LIB_HM_LIB%\mysqlclient.lib
  REM xcopy /Y /E %BUILD_HOME%\plugins\libmysql.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libmysql.lib
  copy /Y %BUILD_HOME%\plugins\libmysql.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libmysql.dll
) else (
  xcopy /Y /E %BUILD_HOME%\dep\mysql\my_default.h %DOWNLOAD_HOME%\%LIB_NAME%\include
  copy /Y  %BUILD_HOME%\plugins\64\mysqlclient.lib %LIBRARY_HOME%\%LIB_HM_LIB%\mysqlclient.lib
  REM xcopy /Y /E %BUILD_HOME%\plugins\x64\libmysql.lib %LIBRARY_HOME%\%LIB_HM_LIB%\libmysql.lib
  copy /Y  %BUILD_HOME%\plugins\64\libmysql.dll %LIBRARY_HOME%\%LIB_HM_LIB%\libmysql.dll
)
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\libmysql.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
REM set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\libmysql.lib
REM IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\mysqlclient.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_MYSQL_BUILD


:TO_CLAPACK_BUILD
echo.
echo #####################################################
echo #   build CLAPACK
echo #####################################################
echo.
set LIB_NAME=CLAPACK-3.1.1-VisualStudio
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/CLAPACK-3.1.1-VisualStudio.zip"

REM # We only test for a small subset of all the CLPACK generated libraries
set ALL_EXIST=true
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\LIB\Win32\BLAS.lib set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\LIB\Win32\libf2c.lib set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\LIB\Win32\clapack.lib set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\LIB\x64\BLAS.lib set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\LIB\x64\libf2c.lib set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\LIB\x64\clapack.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_CLAPACK_BUILD
)

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%/%LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

cd %DOWNLOAD_HOME%\%LIB_NAME%

xcopy /E /Y %BUILD_HOME%\dep\%LIB_NAME%  %DOWNLOAD_HOME%\%LIB_NAME%

if %GDA_BUILD% == BUILD_32 (
  %MSBUILD_EXE% clapack.sln /t:libf2c /property:Configuration="ReleaseDLL"
  %MSBUILD_EXE% clapack.sln /t:BLAS\blas /property:Configuration="ReleaseDLL"
  %MSBUILD_EXE% clapack.sln /t:clapack /property:Configuration="ReleaseDLL"
  REM %MSBUILD_EXE% clapack.sln /t:libf2c /property:Configuration="Debug"
  REM %MSBUILD_EXE% clapack.sln /t:BLAS\blas /property:Configuration="Debug"
  REM %MSBUILD_EXE% clapack.sln /t:clapack /property:Configuration="Debug"
) else (
  %MSBUILD_EXE% clapack.sln /t:libf2c /property:Configuration="ReleaseDLL" /p:Platform="x64"
  REM %MSBUILD_EXE% clapack.sln /t:tmglib /property:Configuration="ReleaseDLL"
  %MSBUILD_EXE% clapack.sln /t:BLAS\blas /property:Configuration="ReleaseDLL" /p:Platform="x64"
  %MSBUILD_EXE% clapack.sln /t:clapack /property:Configuration="ReleaseDLL" /p:Platform="x64"
)
REM # We only test for a small subset of all the CLPACK generated libraries
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\LIB\Win32\BLAS.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\LIB\Win32\libf2c.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\LIB\Win32\clapack.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\LIB\x64\BLAS.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\LIB\x64\libf2c.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\LIB\x64\clapack.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_CLAPACK_BUILD


:TO_WXWIDGETS_BUILD
echo.
echo #####################################################
echo #   build wxWidgets 
echo #####################################################
echo.
set LIB_NAME=wxWidgets-3.1.0
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/wxWidgets-3.1.0.7z"

REM # We are only checking for a small subset of wxWidgets libraries
set ALL_EXIST=true
set WX_DLL_PATH=vc_dll
if %GDA_BUILD% == BUILD_32 (
  if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\lib\vc_dll\wxmsw31u.lib set ALL_EXIST=false
  if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\lib\vc_dll\wxmsw31ud.lib set ALL_EXIST=false
  set WX_DLL_PATH=vc_dll
) else (
  if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\lib\vc_x64_dll\wxmsw31u.lib set ALL_EXIST=false
  if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\lib\vc_x64_dll\wxmsw31ud.lib set ALL_EXIST=false
  set WX_DLL_PATH=vc_x64_dll
)
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_WXWIDGETS_BUILD
)

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.7z %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.7z
    %UNZIP_EXE% %LIB_NAME%.7z -o%DOWNLOAD_HOME%\%LIB_NAME%
)

REM # This applies a patch to src/msw/main.cpp to remove a function
REM # that declares wxWidgets is high-DPI display aware.  wxWidgets
REM # isn't high-DPI display aware and we actually want Windows 8.1
REM # to apply pixel scaling so that the layout of windows isn't messed
REM # up.  OSX already handles Retina displays properly.
xcopy /E /Y %BUILD_DEP%\%LIB_NAME% %DOWNLOAD_HOME%\%LIB_NAME%

cd %DOWNLOAD_HOME%\%LIB_NAME%\build\msw
set WX_HOME=%DOWNLOAD_HOME%\%LIB_NAME%

if %GDA_BUILD% == BUILD_32 (
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=debug MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=release MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1
)  else (
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=debug MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1 TARGET_CPU=AMD64
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=release MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1 TARGET_CPU=AMD64
)
REM # Even though we are skipping this part, the code is still wrong. The name of the dll should be 310u/310ud rather than 31u/31ud.
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\lib\%WX_DLL_PATH%\wxmsw31u_vc_custom.dll %LIBRARY_HOME%\%LIB_HM_LIB%\wxmsw31u_vc_custom.dll
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\lib\%WX_DLL_PATH%\wxmsw31u_gl_vc_custom.dll %LIBRARY_HOME%\%LIB_HM_LIB%\wxmsw31u_gl_vc_custom.dll
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\lib\%WX_DLL_PATH%\wxmsw31ud_vc_custom.dll %LIBRARY_HOME%\%LIB_HM_LIB%\wxmsw31ud_vc_custom.dll
copy /Y %DOWNLOAD_HOME%\%LIB_NAME%\lib\%WX_DLL_PATH%\wxmsw31ud_gl_vc_custom.dll %LIBRARY_HOME%\%LIB_HM_LIB%\wxmsw31ud_gl_vc_custom.dll

REM # We are only checking for a small subset of wxWidgets libraries
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\lib\%WX_DLL_PATH%\wxmsw31u.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\lib\%WX_DLL_PATH%\wxmsw31ud.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_WXWIDGETS_BUILD

:TO_EXPAT_BUILD
echo.
echo #####################################################
echo #   build EXPAT
echo #####################################################
echo.
set LIB_NAME=expat-2.1.0
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/expat-2.1.0.tar.gz"

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\expat.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_EXPAT_BUILD
)

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%/%LIB_NAME% (
    %CURL_EXE% -k -# %LIB_URL% > %LIB_NAME%.tar.gz
    %UNZIP_EXE% %LIB_NAME%.tar.gz
    %UNZIP_EXE% %LIB_NAME%.tar
)

cd %LIB_NAME%

md build
cd build
if %GDA_BUILD% == BUILD_32 (
  cmake .. -G "Visual Studio 10"
  %MSBUILD_EXE% expat.sln /t:ALL_BUILD /property:Configuration="Release" /p:Platform="Win32"
) else (
  cmake .. -G "Visual Studio 10 Win64"
  %MSBUILD_EXE% expat.sln /t:ALL_BUILD /property:Configuration="Release" /p:Platform="x64"
)
copy /Y Release\expat.* %LIBRARY_HOME%\lib
copy /Y ..\lib\*.h %LIBRARY_HOME%\include

set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\expat.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_EXPAT_BUILD

:TO_GDAL_OGR_BUILD
echo.
echo #####################################################
echo #   build GDAL/OGR
echo #####################################################
echo.
set LIB_NAME=GeoDa17Merge
set LIB_URL="https://codeload.github.com/lixun910/gdal/zip/GeoDa17Merge"

set ALL_EXIST=true
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\gdal.lib set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\gdal_geoda20.dll set ALL_EXIST=false
if NOT EXIST %LIBRARY_HOME%\%LIB_HM_LIB%\gdal_i.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_GDAL_OGR_BUILD
)

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%/gdal-%LIB_NAME% (
    %CURL_EXE% -k -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
    move /Y gdal-%LIB_NAME%/gdal gdal
)

cd gdal
if %GDA_BUILD% == BUILD_32 (
  copy /Y %BUILD_HOME%\dep\gdal-1.9.2\nmake.opt nmake.opt
) else (
  copy /Y %BUILD_HOME%\dep\gdal-1.9.2\nmake64.opt nmake.opt
)
copy /Y %BUILD_HOME%\dep\gdal-1.9.2\port\cpl_config.h port\cpl_config.h

nmake -f makefile.vc MSVC_VER=1600
REM nmake -f makefile.vc MSVC_VER=1600 DEBUG=1

copy /Y %DOWNLOAD_HOME%\gdal\gcore\*.h %LIBRARY_HOME%\include
copy /Y %DOWNLOAD_HOME%\gdal\port\*.h %LIBRARY_HOME%\include
copy /Y %DOWNLOAD_HOME%\gdal\ogr\*.h %LIBRARY_HOME%\include
copy /Y %DOWNLOAD_HOME%\gdal\ogr\ogrsf_frmts\*.h %LIBRARY_HOME%\include

copy /Y %DOWNLOAD_HOME%\gdal\gdal_geoda20.dll %LIBRARY_HOME%\%LIB_HM_LIB%\gdal_geoda20.dll
copy /Y %DOWNLOAD_HOME%\gdal\gdal_i.lib %LIBRARY_HOME%\%LIB_HM_LIB%\gdal_i.lib
copy /Y %DOWNLOAD_HOME%\gdal\gdal.lib %LIBRARY_HOME%\%LIB_HM_LIB%\gdal.lib

echo Note: building of OGR plugins skipped by default.  See readme.txt
echo       for information on how to build plugins.
REM call build_ogr_plugins.bat

set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\gdal.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\gdal_geoda20.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%LIBRARY_HOME%\%LIB_HM_LIB%\gdal_i.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_GDAL_OGR_BUILD

:TO_BOOST_BUILD
echo.
echo #####################################################
echo #   build Boost 1.57
echo #####################################################
echo.
set LIB_NAME=boost_1_57_0
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/boost_1_57_0.zip"

set ALL_EXIST=true
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\libboost_thread-vc100-mt-1_57.lib set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\libboost_thread-vc100-mt-gd-1_57.lib set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\boost_chrono-vc100-mt-1_57.dll set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\boost_thread-vc100-mt-1_57.dll set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\boost_system-vc100-mt-1_57.dll set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_BOOST_BUILD
)

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)
echo %DOWNLOAD_HOME%\%LIB_NAME%
set BOOST_HOME=%DOWNLOAD_HOME%\%LIB_NAME%
echo BOOST_HOME: %BOOST_HOME%
cd %BOOST_HOME%
call bootstrap.bat
if %GDA_BUILD% == BUILD_32 (
  call b2 --with-thread --with-date_time --with-chrono --with-system --toolset=msvc-10.0 --build-type=complete stage
  call b2 --with-thread --with-date_time --with-chrono --with-system --toolset=msvc-10.0 --build-type=complete --debug-symbols=on stage
) else (
  call b2 --with-thread --with-date_time --with-chrono --with-system --toolset=msvc-10.0 --build-type=complete architecture=x86 address-model=64 stage
  call b2 --with-thread --with-date_time --with-chrono --with-system --toolset=msvc-10.0 --build-type=complete --debug-symbols=on architecture=x86 address-model=64 stage
)
cd %BUILD_HOME%

set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\libboost_thread-vc100-mt-1_57.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\libboost_thread-vc100-mt-gd-1_57.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\boost_chrono-vc100-mt-1_57.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\boost_thread-vc100-mt-1_57.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\stage\lib\boost_system-vc100-mt-1_57.dll
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_BOOST_BUILD


:TO_JSON_SPIRIT_BUILD
echo.
echo #####################################################
echo #   build JSON Spirit
echo #####################################################
echo.
set LIB_NAME=json_spirit_v4.08
set LIB_URL="https://s3.us-east-2.amazonaws.com/geodabuild/json_spirit_v4.08.zip"

set ALL_EXIST=true
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\Release\json_spirit_lib.lib set ALL_EXIST=false
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME%\Debug\json_spirit_lib.lib set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_JSON_SPIRIT_BUILD
)
cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)
xcopy /Y /E %BUILD_DEP%\json_spirit %DOWNLOAD_HOME%\%LIB_NAME%
cd %LIB_NAME%
if %GDA_BUILD% == BUILD_32 (
  %MSBUILD_EXE% json.sln /property:Configuration="Release" /p:Platform="Win32"
  %MSBUILD_EXE% json.sln /property:Configuration="Debug" /p:Platform="Win32"
) else (
  %MSBUILD_EXE% json.sln /property:Configuration="Release" /p:Platform="x64"
  %MSBUILD_EXE% json.sln /property:Configuration="Debug" /p:Platform="x64"
)
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\Release\json_spirit_lib.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\Debug\json_spirit_lib.lib
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_JSON_SPIRIT_BUILD


:TO_EIGEN3_BUILD
echo.
echo #####################################################
echo #   build  Eigen3
echo #####################################################
echo.
set LIB_NAME=eigen3
set LIB_URL="https://bitbucket.org/eigen/eigen/get/3.3.3.zip"

set ALL_EXIST=true
if NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME% set ALL_EXIST=false
if %ALL_EXIST% == true (
  echo All %LIB_NAME% library targets exist, skipping build
  goto SKIP_EIGEN3_BUILD
)
cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
    move eigen-eigen-* eigen3
)

set CHK_LIB=%DOWNLOAD_HOME%\%LIB_NAME%\Eigen
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
:SKIP_EIGEN3_BUILD

:TO_VISUAL_STUDIO_SETUP_BUILD
echo.
echo #####################################################
echo #   Copy over some DLLs to Debug/Release dirs for
echo #   convenience when using Visual Studio
echo #   to build/debug interactively.
echo #####################################################
echo.
set LIB_NAME=visual_studio_setup
cd %BUILD_HOME%

IF NOT EXIST %BUILD_HOME%\Debug md %BUILD_HOME%\Debug
IF NOT EXIST %BUILD_HOME%\Release md %BUILD_HOME%\Release

copy /Y ..\CommonDistFiles\cache.sqlite Debug\.
copy /Y ..\CommonDistFiles\geoda_prefs.sqlite Debug\.
copy /Y ..\CommonDistFiles\geoda_prefs.json Debug\.
xcopy /I /S /E /Y ..\CommonDistFiles\web_plugins Debug\web_plugins
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\expat.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\gdal_geoda20.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libpq.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\ssleay32.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libeay32.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\spatialite.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\proj.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\geos_c.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\freexl.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\geos_c.dll Debug\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.dll Debug\.
if %GDA_BUILD% == BUILD_32 (
  copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libintl.dll Debug\.
) else (
  copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libintl-8.dll Debug\.
)
copy /Y temp\wxWidgets-3.1.0\lib\vc_x64_dll\wxmsw310ud_vc_custom.dll Debug\.
copy /Y temp\wxWidgets-3.1.0\lib\vc_x64_dll\wxmsw310ud_gl_vc_custom.dll Debug\.
copy /Y temp\boost_1_57_0\stage\lib\boost_chrono-vc100-mt-1_57.dll Debug\.
copy /Y temp\boost_1_57_0\stage\lib\boost_thread-vc100-mt-1_57.dll Debug\.
copy /Y temp\boost_1_57_0\stage\lib\boost_system-vc100-mt-1_57.dll Debug\.

copy /Y ..\CommonDistFiles\cache.sqlite Release\.
copy /Y ..\CommonDistFiles\geoda_prefs.sqlite Release\.
copy /Y ..\CommonDistFiles\geoda_prefs.json Release\.
xcopy /I /S /E /Y ..\CommonDistFiles\web_plugins Release\web_plugins
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\expat.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\gdal_geoda20.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libpq.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\ssleay32.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libeay32.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libcurl.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\spatialite.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\proj.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\geos_c.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\freexl.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\geos_c.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\sqlite.dll Release\.
if %GDA_BUILD% == BUILD_32 (
  copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libintl.dll Release\.
) else (
  copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\libintl-8.dll Release\.
)
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\wxmsw31u_vc_custom.dll Release\.
copy /Y %LIBRARY_HOME%\%LIB_HM_LIB%\wxmsw31u_gl_vc_custom.dll Release\.
copy /Y temp\boost_1_57_0\stage\lib\boost_chrono-vc100-mt-1_57.dll Release\.
copy /Y temp\boost_1_57_0\stage\lib\boost_thread-vc100-mt-1_57.dll Release\.
copy /Y temp\boost_1_57_0\stage\lib\boost_system-vc100-mt-1_57.dll Release\.

md Release\basemap_cache
md Debug\basemap_cache
:SKIP_VISUAL_STUDIO_SETUP_BUILD


:TO_GEODA_BUILD
echo.
echo #####################################################
echo #   build GeoDa %GDA_VERSION%
echo #####################################################
echo.
set LIB_NAME=geoda_build
cd %BUILD_HOME%

IF EXIST %BUILD_HOME%\Release\GeoDa.lib del %BUILD_HOME%\Release\GeoDa.lib
IF EXIST %BUILD_HOME%\Release\GeoDa.exp del %BUILD_HOME%\Release\GeoDa.exp
IF EXIST %BUILD_HOME%\Release\GeoDa.exp del %BUILD_HOME%\Release\GeoDa.exe
IF EXIST %BUILD_HOME%\Debug\GeoDa.lib del %BUILD_HOME%\Debug\GeoDa.lib
IF EXIST %BUILD_HOME%\Debug\GeoDa.exp del %BUILD_HOME%\Debug\GeoDa.exp
IF EXIST %BUILD_HOME%\Debug\GeoDa.exp del %BUILD_HOME%\Debug\GeoDa.exe
if %GDA_BUILD% == BUILD_32 (
  %MSBUILD_EXE% GeoDa.vs2010.sln /t:GeoDa /property:Configuration="Release" /p:Platform="Win32"
  %MSBUILD_EXE% GeoDa.vs2010.sln /t:GeoDa /property:Configuration="Debug" /p:Platform="Win32"
) else (
  %MSBUILD_EXE% GeoDa.vs2010.sln /t:GeoDa /property:Configuration="Release" /p:Platform="x64"
  %MSBUILD_EXE% GeoDa.vs2010.sln /t:GeoDa /property:Configuration="Debug" /p:Platform="x64"
)
set CHK_LIB=%BUILD_HOME%\Release\GeoDa.lib
REM IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%BUILD_HOME%\Release\GeoDa.exp
REM IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%BUILD_HOME%\Release\GeoDa.exe
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%BUILD_HOME%\Debug\GeoDa.lib
REM IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%BUILD_HOME%\Debug\GeoDa.exp
REM IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
set CHK_LIB=%BUILD_HOME%\Debug\GeoDa.exe
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END

:SKIP_GEODA_BUILD


:TO_PACKAGE_BUILD
echo.
echo #####################################################
echo #   Creating GeoDa %GDA_VERSION% installer... 
echo #####################################################
echo.
set LIB_NAME=geoda_package
cd %BUILD_HOME%

set INNO_EXE=not_found
IF EXIST "\Program Files (x86)\Inno Setup 5\ISCC.exe" (
	set INNO_EXE="\Program Files (x86)\Inno Setup 5\ISCC.exe"
)
IF EXIST "\Program Files\Inno Setup 5\ISCC.exe" (
	set INNO_EXE="\Program Files\Inno Setup 5\ISCC.exe"
)
if %INNO_EXE% == not_found (
	echo Could not find location of Inno 5 command line installer ISCC.exe.  Please install Inno 5.
	goto BUILD_END
)

IF EXIST %BUILD_HOME%\geoda_setup.exe del %BUILD_HOME%\geoda_setup.exe
if %GDA_BUILD% == BUILD_32 (
	%INNO_EXE% /q installer\32bit\GeoDa.iss
) else (
	%INNO_EXE% /q installer\64bit\GeoDa.iss
)
set CHK_LIB=%BUILD_HOME%\geoda_setup.exe
IF NOT EXIST %CHK_LIB% goto MISSING_TARGET_END
if %GDA_BUILD% == BUILD_32 (
	move /Y %BUILD_HOME%\geoda_setup.exe %BUILD_HOME%\GeoDa-%GDA_VERSION%-Windows-32bit.exe
) else (
	move /Y %BUILD_HOME%\geoda_setup.exe %BUILD_HOME%\GeoDa-%GDA_VERSION%-Windows-64bit.exe	
)
:SKIP_PACKAGE_BUILD

echo Finished building and packaging GeoDa %GDA_VERSION%

:BUILD_END
goto REAL_END

:MISSING_TARGET_END
cd %BUILD_HOME%
echo.
echo Build stopped while building %LIB_NAME% due to
echo missing target: %CHK_LIB%
echo.

:REAL_END
