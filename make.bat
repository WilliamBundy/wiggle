
@echo off
set msvcdir="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\"
if not defined DevEnvDir call %msvcdir%vcvars64.bat >nul
cl /nologo /TP /W3 /wd4244 /EHsc /I"src/include" /MD /O2 src\main.cc /Fe"bin/wiggle.exe" /link /nologo src\freetype27.lib shlwapi.lib /INCREMENTAL:NO /SUBSYSTEM:CONSOLE

del *.obj 2>&1 >nul

rem start wiggle DejaVuSansCondensed.ttf dvsc.wgd
