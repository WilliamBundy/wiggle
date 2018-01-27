@echo off

set msvcdir="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\"
if not defined DevEnvDir call %msvcdir%vcvars64.bat >nul

cl  /nologo ^
	/TC ^
	/EHsc ^
	/Zi ^
	/W3 ^
	/Gs16777216 ^
	/GS- ^
	/Gm- ^
	/fp:fast ^
	/wd4477 ^
	/wd4244 ^
	/wd4267 ^
	/wd4334 ^
	/wd4305 ^
	/wd4101 ^
	main.c ^
	/Fe"TextInvaders.exe" ^
	/Fd"TextInvaders.pdb" ^
/link ^
	/nologo ^
	/STACK:16777216,16777216 ^
	/entry:TextInvadersMain ^
	/NODEFAULTLIB ^
	kernel32.lib gdi32.lib user32.lib opengl32.lib ^
	/SUBSYSTEM:WINDOWS ^
	/INCREMENTAL:NO

del *.obj 2>&1 >nul

