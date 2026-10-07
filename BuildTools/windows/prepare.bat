@echo OFF
setlocal EnableDelayedExpansion

set VS_VER=2019
set MSC_VER=1900
set MSVC++=19.0

if %PROCESSOR_ARCHITECTURE% == x86 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
    
) else if %PROCESSOR_ARCHITECTURE% == AMD64 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if %PROCESSOR_ARCHITECTURE% == ARM64 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsarm64.bat"
)


for /f "tokens=4 delims=;= " %%P in ('findstr /c:"version_major" ..\..\version.h') do set VER_MAJOR=%%P
for /f "tokens=4 delims=;= " %%P in ('findstr /c:"version_minor" ..\..\version.h') do set VER_MINOR=%%P
for /f "tokens=4 delims=;= " %%P in ('findstr /c:"version_build" ..\..\version.h') do set VER_BUILD=%%P
for /f "tokens=4 delims=;= " %%P in ('findstr /c:"version_subbuild" ..\..\version.h') do set VER_SUBBUILD=%%P

set GDA_VERSION=%VER_MAJOR%.%VER_MINOR%.%VER_BUILD%.%VER_SUBBUILD%

echo.
echo #####################################################
echo #
echo #  Build script for Windows 32-bit and 64-bit
echo #
echo #  PROCESSOR_ARCHITECTURE: %PROCESSOR_ARCHITECTURE%

set GDA_BUILD=unknown
if %PROCESSOR_ARCHITECTURE% == x86 (
  echo #  32-bit Windows detected
  set GDA_BUILD=BUILD_32
) else if %PROCESSOR_ARCHITECTURE% == AMD64 (
  echo #  64-bit Windows detected
  set GDA_BUILD=BUILD_x64
) else if %PROCESSOR_ARCHITECTURE% == ARM64 (
  echo #  64-bit Windows detected
  set GDA_BUILD=BUILD_a64
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

set CURL_EXE=%BUILD_DEP%\curl.exe -k -L
echo CURL_EXE: %CURL_EXE%

set MSBUILD_EXE= msbuild
echo MSBUILD_EXE: %MSBUILD_EXE%

IF NOT EXIST %DOWNLOAD_HOME% md %DOWNLOAD_HOME%

if NOT %GDA_BUILD% == BUILD_a64 (
  echo.
  echo #####################################################
  echo #   build  GDAL
  echo #####################################################
  echo.
  set LIB_NAME=release-1911
  set LIB_URL="https://github.com/GeoDaCenter/software/releases/download/v2000/%LIB_NAME%-dev.zip" 

  cd %DOWNLOAD_HOME%
  IF NOT EXIST %DOWNLOAD_HOME%/%LIB_NAME% (
      IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
      %UNZIP_EXE% %LIB_NAME%.zip
      move release-1911 ..\libraries
  )
)

echo.
echo #####################################################
echo #   build CLAPACK
echo #####################################################
echo.
set LIB_NAME=CLAPACK-3.1.1-VisualStudio
set LIB_URL="https://github.com/GeoDaCenter/software/releases/download/v2000/CLAPACK-3.1.1-VisualStudio.zip"

cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%/%LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

cd %DOWNLOAD_HOME%\%LIB_NAME%

xcopy /E /Y %BUILD_HOME%\dep\%LIB_NAME%  %DOWNLOAD_HOME%\%LIB_NAME%

if %GDA_BUILD% == BUILD_a64 (
  %MSBUILD_EXE% clapack_vs2022.sln /t:libf2c_vs2022 /property:Configuration="ReleaseDLL" /p:Platform="ARM64"
  %MSBUILD_EXE% clapack_vs2022.sln /t:BLAS\blas_vs2022 /property:Configuration="ReleaseDLL" /p:Platform="ARM64"
  %MSBUILD_EXE% clapack_vs2022.sln /t:clapack_vs2022 /property:Configuration="ReleaseDLL" /p:Platform="ARM64"
) else if %GDA_BUILD% == BUILD_32 (
  %MSBUILD_EXE% clapack.sln /t:libf2c /property:Configuration="ReleaseDLL" /p:Platform="Win32"
  %MSBUILD_EXE% clapack.sln /t:BLAS\blas /property:Configuration="ReleaseDLL" /p:Platform="Win32"
  %MSBUILD_EXE% clapack.sln /t:clapack /property:Configuration="ReleaseDLL" /p:Platform="Win32"
) else (
  %MSBUILD_EXE% clapack.sln /t:libf2c /property:Configuration="ReleaseDLL" /p:Platform="x64"
  %MSBUILD_EXE% clapack.sln /t:BLAS\blas /property:Configuration="ReleaseDLL" /p:Platform="x64"
  %MSBUILD_EXE% clapack.sln /t:clapack /property:Configuration="ReleaseDLL" /p:Platform="x64"
)

echo.
echo #####################################################
echo #   build wxWidgets 
echo #####################################################
echo.
set LIB_NAME=wxWidgets-3.1.4
set LIB_URL="https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.4/wxWidgets-3.1.4.zip"


cd %DOWNLOAD_HOME%

IF NOT EXIST %DOWNLOAD_HOME%\%LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.7z %CURL_EXE% -L %LIB_URL% > %LIB_NAME%.7z
    %UNZIP_EXE% %LIB_NAME%.7z -o%DOWNLOAD_HOME%\%LIB_NAME%
    xcopy %BUILD_DEP%\wxWidgets\* %DOWNLOAD_HOME%\%LIB_NAME% /E /I /H /Y
)

cd %DOWNLOAD_HOME%\%LIB_NAME%\build\msw
set WX_HOME=%DOWNLOAD_HOME%\%LIB_NAME%

if %GDA_BUILD% == BUILD_a64 (
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=debug MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1 TARGET_CPU=ARM64
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=release MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1 TARGET_CPU=ARM64
) else if %GDA_BUILD% == BUILD_32 (
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=debug MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=release MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1
)  else (
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=debug MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1 TARGET_CPU=AMD64
  nmake -f makefile.vc UNICODE=1 SHARED=1 RUNTIME_LIBS=dynamic BUILD=release MONOLITHIC=1 USE_OPENGL=1 USE_POSTSCRIPT=1 TARGET_CPU=AMD64
)
xcopy "%DOWNLOAD_HOME%\%LIB_NAME%" "%DOWNLOAD_HOME%\wxWidgets\" /E /I /H /Y

:TO_BOOST_BUILD
cd %DOWNLOAD_HOME%

if %GDA_BUILD% == BUILD_a64 (
  echo.
  echo #####################################################
  echo #   build Boost 1.88
  echo #####################################################
  echo.
  set LIB_NAME=boost-1.88.0
  set LIB_URL="https://github.com/boostorg/boost.git"

  IF NOT EXIST %DOWNLOAD_HOME%\!LIB_NAME!\.git\  (
    rmdir /s /q !LIB_NAME!
    git clone !LIB_URL! !LIB_NAME!
    pushd !LIB_NAME!
      git checkout !LIB_NAME!
      git submodule update --init --recursive
    popd
  )
  pushd !LIB_NAME!
    call bootstrap.bat
    b2.exe -j12 toolset=msvc architecture=arm address-model=64 abi=ms pch=off --without-mpi --without-graph_parallel install --prefix=%DOWNLOAD_HOME%\boost
    robocopy "%DOWNLOAD_HOME%\boost\include\boost-1_88\boost" "%DOWNLOAD_HOME%\boost\include\boost" /E /MOVE
    rmdir /s /q "%DOWNLOAD_HOME%\boost\include\boost-1_88"
  popd
) else (
  echo.
  echo #####################################################
  echo #   build Boost 1.75
  echo #####################################################
  echo.
  set LIB_NAME=pygeoda_boost-1.18
  set LIB_URL="https://github.com/lixun910/pygeoda_boost/archive/refs/tags/v1.18.zip"

  IF NOT EXIST %DOWNLOAD_HOME%\!LIB_NAME!  (
      IF NOT EXIST v1.18.zip %CURL_EXE% -# !LIB_URL! > v1.18.zip
      %UNZIP_EXE% !LIB_NAME!.zip
      ren pygeoda_boost-1.18 boost
  )
)

echo.
echo #####################################################
echo #   build JSON Spirit
echo #####################################################
echo.
set LIB_NAME=json_spirit_v4.08
set LIB_URL="https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip"

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

xcopy /Y /E %BUILD_DEP%\json_spirit %DOWNLOAD_HOME%\%LIB_NAME%
cd %LIB_NAME%
if %GDA_BUILD% == BUILD_a64 (
  %MSBUILD_EXE% json_vs2022.sln /property:Configuration="Release" /p:Platform="ARM64"
  %MSBUILD_EXE% json_vs2022.sln /property:Configuration="Debug" /p:Platform="ARM64"
) else if %GDA_BUILD% == BUILD_32 (
  %MSBUILD_EXE% json.sln /property:Configuration="Release" /p:Platform="Win32"
  %MSBUILD_EXE% json.sln /property:Configuration="Debug" /p:Platform="Win32"
) else (
  %MSBUILD_EXE% json.sln /property:Configuration="Release" /p:Platform="x64"
  %MSBUILD_EXE% json.sln /property:Configuration="Debug" /p:Platform="x64"
)

echo.
echo #####################################################
echo #   build  Eigen3
echo #####################################################
echo.
set LIB_NAME=eigen3
set LIB_URL="https://github.com/GeoDaCenter/software/releases/download/v2000/eigen3.zip"

cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
)

echo.
echo #####################################################
echo #   build  Spectra
echo #####################################################
echo.
set LIB_NAME=spectra
set LIB_URL="https://github.com/yixuan/spectra/archive/refs/tags/v0.8.0.zip"
cd %DOWNLOAD_HOME%
IF NOT EXIST %LIB_NAME% (
    IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
    %UNZIP_EXE% %LIB_NAME%.zip
    ren spectra-0.8.0 spectra
)

if NOT %GDA_BUILD% == BUILD_a64 (
  echo.
  echo #####################################################
  echo #   build  OpenCL
  echo #####################################################
  echo.
  set LIB_NAME=OpenCL
  set LIB_URL="https://github.com/GeoDaCenter/software/releases/download/v2000/OpenCL.zip"
  cd %DOWNLOAD_HOME%
  IF NOT EXIST %LIB_NAME% (
      IF NOT EXIST %LIB_NAME%.zip %CURL_EXE% -# %LIB_URL% > %LIB_NAME%.zip
      %UNZIP_EXE% %LIB_NAME%.zip
  )
)

if %GDA_BUILD% == BUILD_a64 (
  cd %BUILD_HOME%
  IF NOT EXIST %BUILD_HOME%\vcpkg\.git\  (
    rmdir /s /q vcpkg
    git clone https://github.com/microsoft/vcpkg.git vcpkg
    pushd vcpkg
      call "bootstrap-vcpkg.bat"
    popd
  )
  pushd vcpkg
    vcpkg install gdal:arm64-windows opencl:arm64-windows --x-install-root=%BUILD_HOME%
  popd
  robocopy "%BUILD_HOME%\arm64-windows" "%BUILD_HOME%\libraries" /E /MOVE
)
