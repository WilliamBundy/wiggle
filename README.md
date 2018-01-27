## Wiggle
### A one-stop solution for SDF font atlas generation
#### (warning: it's a mess)

## Build instructions

Wiggle comes with all its dependencies. Wiggle is currently Win32 only.

1. Have Visual Studio installed
2. Run make.bat in a developer prompt 
3. Wiggle.exe should work if it's in bin/

## Description

Wiggle constitutes the quickest and dirtiest attempt to build some tooling to aid with making nice looking text in games. This utility is largely cobbled together from parts, behaves more like a function than a program, has some gross inefficiences, and probably crashes if you feed it invalid input. However, it does work; it wraps msdfgen, freetype, and Adobe's kerndump script to generate image atlases of fonts using stb_image with a only a few invocations. 

eg:
```
set wiggle=bin\wiggle
%wiggle% fontgen font1.otf font1.wfi font1.png 128 96 4 8 4 8 1
%wiggle% fontgen font2.otf font2.wfi font2.png 128 96 4 8 4 8 1
%wiggle% atlas fonts.png 1024 font1.png font1.wfi font2.png font2.wfi
%wiggle% rgba fonts.png fonts.rgba
```

In detail, the `fontgen` command is the star of the show: it calls out to the other programs to render glyphs, generate metrics, and gather kerning, then it atlases the results and writes it out. Once each font has been rendered to its own atlas, `atlas` gathers all the existing font atlases and writes them to a bigger atlas, along with any other images you add. This makes it easier to ship a project using only a single texture. Finally, the `rgba` command converts an encoded image (anything supported by stb_image should work) to raw 32-bit rgba data, for ease of loading in minimal applications.