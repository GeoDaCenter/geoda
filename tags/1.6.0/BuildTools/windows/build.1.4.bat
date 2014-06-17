@echo ON
call "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
call "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
call "C:\Program Files\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"

set BUILD_HOME=%CD%
set DOWNLOAD_HOME=%BUILD_HOME%\temp
set BUILD_DEP=%BUILD_HOME%\dep
set UNZIP_EXE=%BUILD_DEP%\7za.exe x
set CURL_EXE=%BUILD_DEP%\curl.exe
set MSBUILD_EXE=msbuild
  
IF NOT EXIST %DOWNLOAD_HOME% (
    md %DOWNLOAD_HOME%
)

REM #####################################################
REM #   get Boost
REM #####################################################
set LIB_NAME="boost_1_55_0"
set LIB_URL="http://jaist.dl.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.zip"

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%/%LIB_NAME% (
    %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

set BOOST_HOME=%DOWNLOAD_HOME%\%LIB_NAME%

REM #####################################################
REM #   build wxWidget 
REM #####################################################
set LIB_NAME="wxWidgets-2.9.4"
set LIB_URL="http://iweb.dl.sourceforge.net/project/wxwindows/2.9.4/wxWidgets-2.9.4.7z"

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME% (
    %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.7z
    %UNZIP_EXE% %LIB_NAME%.7z -o%DOWNLOAD_HOME%\%LIB_NAME%
)

cd %DOWNLOAD_HOME%\%LIB_NAME%\build\msw
nmake -f makefile.vc UNICODE=1 SHARED=0 RUNTIME_LIBS=static BUILD=debug MONOLITHIC=0 USE_OPENGL=1 USE_POSTSCRIPT=1
nmake -f makefile.vc UNICODE=1 SHARED=0 RUNTIME_LIBS=static BUILD=release MONOLITHIC=0 USE_OPENGL=1 USE_POSTSCRIPT=1

set WX_HOME=%DOWNLOAD_HOME%\%LIB_NAME%

REM #####################################################
REM #   build CLAPACK
REM #####################################################
set LIB_NAME="CLAPACK-3.1.1-VisualStudio"
set LIB_URL="http://www.netlib.org/clapack/CLAPACK-3.1.1-VisualStudio.zip"

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%/%LIB_NAME% (
    %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

cd %DOWNLOAD_HOME%\%LIB_NAME%
REM %MSBUILD_EXE% clapack.sln /property:Configuration="Release"

REM #####################################################
REM #   build GeoDa
REM #####################################################
cd %BUILD_HOME%

%MSBUILD_EXE% GeoDa.vs2008.sln /property:Configuration="Release"

