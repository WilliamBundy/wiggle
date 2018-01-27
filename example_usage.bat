@echo off
set wiggle=bin\wiggle.exe

set sspr=example_files\SourceSerifPro-Regular.otf
set sspl=example_files\SourceSerifPro-Light.otf
set sspb=example_files\SourceSerifPro-Bold.otf

rem                                       width height scale x y  pxrange render
%wiggle% fontgen %sspr% sspr.wfi sspr.png 128   96     4     8 4  4       1
%wiggle% fontgen %sspl% sspl.wfi sspl.png 128   96     4     8 4  4       1
%wiggle% fontgen %sspb% sspb.wfi sspb.png 128   96     4     8 4  4       1

%wiggle% atlas game_atlas.png 2048 ^
	example_files\sprites.png 0 ^
	example_files\background.png 0 ^
	sspr.png sspr.wfi ^
	sspl.png sspl.wfi ^
	sspb.png sspb.wfi ^
	> atlas.txt

%wiggle% rgba game_atlas.png game_atlas.rgba

copy game_atlas.rgba demo\
copy sspr.wfi demo\
