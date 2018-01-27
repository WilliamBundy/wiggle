## Wiggle
### A one-stop solution for SDF font atlas generation
#### (it's a bit of a mess, but it works!)

## Build instructions

Wiggle comes with all its dependencies. Wiggle is currently 64-bit Windows only.

1. Have Visual Studio installed
2. Run make.bat in a developer prompt 
3. Wiggle.exe should work if it's in bin/
4. Run `example_usage.bat`
5. Run the make.bat in the demo folder to build the demo.

## Description

Wiggle is the result of a week of quick-and-dirty coding in an attempt to build some tooling to aid with making nicer looking text in small games. If you think that is a heavily qualified sentence, you're correct. This utility is largely cobbled together from parts, behaves more like a function than a program, and probably crashes if you feed it invalid input. However, it does work; it wraps msdfgen, freetype, and Adobe's kerndump script to generate image atlases of fonts using stb_image with a only a few invocations. 

eg:
```bat
set wiggle=bin\wiggle
%wiggle% fontgen font1.otf font1.wfi font1.png 128 96 4 8 4 8 1
%wiggle% fontgen font2.otf font2.wfi font2.png 128 96 4 8 4 8 1
%wiggle% atlas fonts.png 1024 font1.png font1.wfi font2.png font2.wfi
%wiggle% rgba fonts.png fonts.rgba
```

In detail, the `fontgen` command is the star of the show: it calls out to the other programs to render glyphs, generate metrics, and gather kerning, then it atlases the results and writes it out.

Once each font has been rendered to its own atlas, `atlas` gathers all the existing font atlases and writes them to a bigger atlas, along with any other images you add. This makes it easier to ship a project using only a single texture. 

Finally, the `rgba` command converts an encoded image (anything supported by stb_image should work) to raw 32-bit rgba data, for ease of loading in minimal applications.

## Demo

Text Invaders--the name's influence should be clear--shows off the quality of the rendered text in a little game, built with no external libraries. It uses the results from `example_usage.bat`, so run that first. 

## License

All my code and graphics are "unlicensed" into the public domain, to be used how you see fit. msdfgen, fonttools, and kerndump are all under the MIT license, and FreeType is under the FreeType licence, which can be found in src/include/FTL.txt. Internally, I use stb_image, stb_image_write, stb_rect_pack, and whereami, which are all in the public domain. The Source Serif fonts supplied are under the Open Font License.

## Notes

This thing is a mess; it uses a lot of other programs through `system(...)` calls, and won't work if it's separated from them. It'll overwrite the file `kerning.txt` and things in the folder `wiggleTemp/`. 