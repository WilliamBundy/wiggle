#define _CRT_SECURE_NO_WARNINGS
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include <Windows.h>
#include <Shlwapi.h>

#include "ft2build.h"
#include FT_FREETYPE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "whereami.h"
#include "whereami.c"

#include "stuff.cc"

#pragma pack(push, 4)
struct Glyph
{
	int character;
	float width, height;
	float x, y;
	float advance;
	float l, b, r, t;
};

struct GlyphImage
{
	i32 x, y, w, h;
	float bbx, bby;
};

struct FontInfo
{
	i32 sizeX, sizeY;
	i32 scale;
	i32 offsetX, offsetY;
	i32 pxRange;
	i32 lineSpacing;
	i32 atlasX, atlasY;

	Glyph glyphs[96];
	GlyphImage images[96];
	float kerning[96][96];
};
#pragma pack(pop)

FontInfo* loadFontInfo(char* filename)
{
	FontInfo* fi = NULL;
	FILE* fp = fopen(filename, "rb");
	if(fp != NULL) {
		fi = (FontInfo*)calloc(sizeof(FontInfo), 1);
		fread(fi, sizeof(FontInfo), 1, fp);
		fclose(fp);
	} else {
		fprintf(stderr, ">>> Could not open file %s\n", filename);
	}
	return fi;
}

string getEnclosingFolder()
{
	int len = wai_getExecutablePath(NULL, 0, NULL);
	char* path = (char*)malloc(len + 2);
	int dirlen;
	wai_getExecutablePath(path, len, &dirlen);
	path[dirlen] = '/';
	path[dirlen+1] = '\0';
	return path;
}

float ptf(unsigned int p)
{
	icvt cc;
	cc.u = p;
	float f = (float)cc.i;
	return f / 64.0f;
}

struct PrintedMetrics
{
	float advance;
	int range;
	float l, b, r, t;
};

FT_Library ft;
char* parsePrintedMetrics(char* text, PrintedMetrics* m)
{
	m->advance = 0;
	m->range = 8;
	//we only want bounds lol
	
	while(text[0] != 'b') text++;
	text += strlen("bounds = ");
	char* end = NULL;
	m->l = strtof(text, &end);
	text = end + 2;
	m->b = strtof(text, &end);
	text = end + 2;
	m->r = strtof(text, &end);
	text = end + 2;
	m->t = strtof(text, &end);
	text = end;
	return text;
}

#include "wiggle_fontgen.cc"
#include "wiggle_atlas.cc"

#define massert(p, m) do{if(!(p)) {fprintf(stderr, m); return 1;}}while(0)
int main(int argc, char** argv)
{
	i32 mode = 0;
	memset(&gkt, 0, sizeof(gkt));
	if(argc >= 2) {
		u64 fontgen = hashString("fontgen");
		u64 atlas = hashString("atlas");
		u64 rgba = hashString("rgba");
		//u64 dumpinfo = hashString("dumpinfo");
		u64 png = hashString("png");


		u64 arg = hashString(argv[1]);
		if(arg == fontgen) {
			mode = 1;
		} else if(arg == atlas) {
			mode = 2;
		} else if(arg == rgba) {
			mode = 3; 
		//} else if(arg == dumpinfo) {
		//	mode = 4;
		} else if(arg == png) {
			mode = 5;
		} else {
			fprintf(stderr, "Error: unknown mode %s\n"
					"Modes are: fontgen, atlas, rgba, dumpinfo, png\n" 
					"or, just `wiggle` to see help\n", 
					argv[1]);
			return 1;
		}
	} else {
		fprintf(stdout, 
				"wiggle -- a font image generating utility\n"
				"modes: \n    fontgen <otf_or_ttf_font_file>.otf <outputfile>.wfi <atlasfile>.png "	
				"ImageWidth ImageHeight FontScale PosX PosY PxRange DoRender\n"
				"        Mode fontgen renders a font with msdf and writes the font metrics to a .wfi file\n"
				"        (yes, that's a lot of arguments; just use `wiggle fontgen` for help)\n\n"
				"    atlas <output>.png SizeInPixels <file1>.png <file1_or_0>.wfi ... <fileN>.png <fileN_or_0>.wfi\n"
				"        The atlas mode combines several images together, and can write a font's \n"
				"        position in the atlas to its corresponding font info file.\n\n"
				"    rgba <image>.png <image2>.rgba \n"
				"        Decodes a png and writes the raw color data in 32 bit rgba form. This is useful if you want something\n"
				"        that's very easy to load, but don't care about how much space it takes up.\n\n"
				"    png <image>.rgba <image2>.png \n"
				"        Converts a .rgba file back into a png\n\n"
				//"    dumpinfo <fontinfo>.wfi\n"
				//"        Dumps the contents of a .wfi file to stdout in plain text\n"
				);

	}

	if(mode == 1) {
		if(argc < 11) {
			fprintf(stderr, "Error: Missing arguments\n"
				"Format: wiggle fontgen <fontname> <outputfile>.wfi "
				"<atlasfile>.png ImageWidth ImageHeight FontScale PosX PosY PxRange DoRender\n");
			fprintf(stderr,
					"Notes: \n"
					"Fontgen works by calling msdfgen for each printable ascii character\n"
					"then loading them, loading the metrics, and atlasing all the characters.\n"
					"Many of these arguments are given directly to msdfgen\n"
					"ImageWidth and ImageHeight are the size of each glyph image. These are\n"
					"cropped on the horizontal to save space, but the entire vertical span\n"
					"is kept to preserve the baseline. Something like 128,80 is a good start\n\n"
					"FontScale is just that; I tend to use 2-4\n"
					"PosX and PosY are the offset of the glyphs within their images.\n"
					"Glyphs often contain negative coordinates, so they need to be moved\n"
					"The amount depends on the scale, 8,8 is often enough though\n"
					"PxRange is the range of the signed distance field. I use 8\n"
					"DoRender needs to be 1. If it's 0, the msdfgen rendering step is skipped\n"
					"This is useful if you need to re-run the packing phase for some reason\n");
			return 1;
		}
		string ff = argv[2];
		string of = argv[3];
		string af = argv[4];
		int sx = atoi(argv[5]);
		int sy = atoi(argv[6]);
		int s = atoi(argv[7]);
		int ox = atoi(argv[8]);
		int oy = atoi(argv[9]);
		int p = atoi(argv[10]);
		int r = atoi(argv[11]);
		massert(sx != 0, "Size cannot be zero");
		massert(sy != 0, "Size cannot be zero");
		massert(s != 0, "Scale cannot be zero");
		massert(p != 0, "Pixel range cannot be zero; >4 is recommended");
		return wiggleGenFonts(ff, of, af, sx, sy, s, ox, oy, p, r);
	}

	if(mode == 2) {
		//argv1 = atlas
		//argv2 = outfile
		//argv3 = size
		//argv4...n = input files
		if(argc < 4) {
			fprintf(stderr, "Error: bad command\n"
					"Format: wiggle atlas <ouptut>.png atlasSize <file1>.png <file1>.wfi...\n\n"
					"If you want to atlas other files, pass zero instead of a wfi file\n"
					"The command writes the positions to stdout, so > to a file.\n");
			return 1;
		}
		char* outfile = argv[2];
		int size = atoi(argv[3]);
		massert(size > 256, "Given size is probably too small");
		char** files = argv + 4;
		int fcount = argc - 4;
		massert(fcount > 1, "Why are you trying to atlas a single image???");
		wiggleGenAtlas(outfile, size, files, fcount);
	}

	if(mode == 3) {
		if(argc < 4) {
			fprintf(stderr, "Error: not enough arguments:\n"
				"    rgba <image>.png <image2>.rgba \n");
		}


		i32 w, h, cc;
		u32* img = (u32*)stbi_load(argv[2], &w, &h, &cc, 4);
		if(w == 0) {
			fprintf(stderr, "couldn't open %s", argv[2]);
			return 1;
		}

		FILE* fi = fopen(argv[3], "wb");
		if(!fi) {
			fprintf(stderr, "Error: could not open %s for writing", argv[3]);
			return 1;
		}
		//assume first two slots are some kind of version number or other info
		//we just leave these zero, it's up to you to fill them
		u32 header[4] = {0, 0, (u32)w, (u32)h};
		fwrite(header, sizeof(u32), 4, fi);
		fwrite(img, sizeof(u32), w * h, fi);
		fclose(fi);
	}

	//dumpinfo
	if(mode == 4) {
#if 0
	int character;
	float width, height;
	float x, y;
	float advance;
	float l, b, r, t;
};

struct GlyphImage
{
	i32 x, y, w, h;
	float bbx, bby;
};

struct FontInfo
{
	i32 sizeX, sizeY;
	i32 scale;
	i32 offsetX, offsetY;
	i32 pxRange;
	i32 lineSpacing;
	i32 atlasX, atlasY;

	Glyph glyphs[96];
	GlyphImage images[96];
	float kerning[96][96];
};
#endif

	}

	//png
	if(mode == 5) {
		if(argc < 3) {
			fprintf(stderr, "Error: not enough arguments:\n"
				"    png <image>.rgba <image2>.png \n");
		}

		FILE* fi = fopen(argv[2], "rb");
		if(!fi) {
			fprintf(stderr, "Error: could not open %s for reading", argv[3]);
			return 1;
		}
		u32 header[4];
		fread(header, sizeof(u32), 4, fi);
		u32* img = (u32*)malloc(header[2] * header[3] * 4 + 64);
		fread(img, sizeof(u32), header[2] * header[3], fi);

		stbi_write_png(argv[3], header[2], header[3], 4, img, header[2] * 4);
	}
	return 0;
}
