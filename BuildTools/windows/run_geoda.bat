@echo OFF
SET geodapath=%cd%
SET GEODA_GDAL_DATA="%geodapath%\data"
SET GEODA_OGR_DRIVER_PATH="%geodapath%"

@echo %geodapath%
@echo %GEODA_GDAL_DATA%
@echo %GEODA_GDAL_DRIVER_PATH%

start GeoDa.exe
