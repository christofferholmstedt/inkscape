@echo Setting environment variables for MinGw build of Inkscape

IF "%DEVLIBS_PATH%"=="" set DEVLIBS_PATH=e:\Inkscape\devlibs
IF "%MINGW_PATH%"=="" set MINGW_PATH=c:\mingw32
set MINGW_BIN=%MINGW_PATH%\bin
set PKG_CONFIG_PATH=%DEVLIBS_PATH%\lib\pkgconfig
set PATH=%DEVLIBS_PATH%\bin;%DEVLIBS_PATH%\python;%MINGW_BIN%;%PATH%
