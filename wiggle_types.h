
typedef struct wGlyph wGlyph;
typedef struct wGlyphImage wGlyphImage;
typedef struct wFontInfo wFontInfo;
typedef struct wRgbaFileHeader wRgbaFileHeader;

#ifndef WIGGLE_NO_TYPES
#ifdef WIGGLE_USE_STDINT
typedef float f32;
typedef int32_t i32;
typedef uint32_t u32;
#else
typedef float f32;
typedef int i32;
typedef unsigned int u32;
#endif
#endif

#pragma pack(push, 4)
struct wGlyph
{
	i32 character;
	f32 width, height;
	f32 x, y;
	f32 advance;
	f32 l, b, r, t;
};

struct wGlyphImage
{
	i32 x, y, w, h;
	f32 bbx, bby;
};

struct wFontInfo
{
	i32 sizeX, sizeY;
	i32 scale;
	i32 offsetX, offsetY;
	i32 pxRange;
	i32 lineSpacing;
	i32 atlasX, atlasY;

	wGlyph glyphs[96];
	wGlyphImage images[96];
	f32 kerning[96][96];
};

struct wRgbaFileHeader
{
	u32 unused[2];
	u32 width, height;
};
#pragma pack(pop)






#if 0
/* Some sample code, for working with these things
 *
 * You should really check out the demo program if you want examples of 
 * this stuff working in action, though.
 *
 * Most of these are extracted from there, though with some changes; these have 
 * had parts removed for simplicity
 */

/* Loading a .rgba file as written by wiggle
 *
 * The header is just there to help you understand how it works; you don't need 
 * it to load the file. The demo program doesn't have access to fopen and kin, so 
 * it uses the Win32 functions to do the same
 */
u32* exampleLoadRgba(char* filename, int* w, int* h)
{
	FILE* fp = fopen(filename, "rb");
	if(!fp) return NULL;

	wRgbaFileHeader header;
	fread(&header, sizeof(wRgbaFileHeader), 1, fp);

	u32* img = (u32*)malloc(header.width * header.height * 4);
	fread(img, 4, header.width * header.height, fp);

	*w = (int)header.width;
	*h = (int)header.height;

	return img;
}

/* Example "drawText" function
 *
 * This is pretty messy; I'm still not sure of all the units. The basic idea is the
 * same as always: loop over your characters, draw them, move right by that one's
 * advance, move left by the kerning pair with the previous one. 
 */

void exampleRenderRect(f32 sdf, u32 color, 
		//Screen position
		f32 x, f32 y, f32 w, f32, h,
		//Texture coords
		f32 tx, f32 ty, f32 tw, f32 th);

void exampleDrawText(wFontInfo* info,
		f32 x, f32 y,
		char* text, isize count,
		f32 pointSize, u32 color, f32 sdfSharpness)
{
	//offsets
	f32 ox = 0.0f, oy = 0.0f;

	//kept track of for kerning
	char last = 0;

	//slightly confused transformations
	//Converting to pixels from points; probably not quite accurate because
	//we're accidentally taking into account things like padding
	//Or, possibly, there's some confusion and disagreement coming from FreeType 
	//and msdfgen about exactly what the units are. That might explain a lot of 
	//the weird units and magic numbers in this function.
	f32 pixelSize = (pointSize * group->dpi) / 72.0f;
	f32 padding = (f32)info->pxRange;
	f32 fontScale = info->scale;

	//This might be part of the issue too; 'A' should be Npts tall, 
	//but not everything else? Supposedly, wiggle outputs its metrics in ems
	//(I did the best I can there, not entirely sure to be honest)
	wGlyphImage* a = NULL;
	wGlyph* g = info->glyphs + ('A'-32);
	f32 glyphHeight = wabsf(g->t - g->b);
	f32 scaledHeight = glyphHeight * fontScale;
	f32 scaledRatio = pixelSize / scaledHeight;
	f32 heightRatio = pixelSize / glyphHeight;

	i32 newLine = 1;
	for(isize i = 0; i < count; ++i) {
		char c = text[i];

		//Incredibly rudimentary handling of special characters
		switch(c) {
			case '\r':
				continue;

			case '\n':
				if(ox > maxX) maxX = ox;
				ox = 0;
				oy += info->lineSpacing * heightRatio;
				newLine = 1;
				continue;

			case '\t':
				ox += info->glyphs[0].advance * heightRatio * 8;
				continue;

			case ' ':
				ox += info->glyphs[0].advance * heightRatio;
				continue;
		}

		if(c <= 32 || c >= 127) continue;
		a = info->images + (c-32);
		g = info->glyphs + (c-32);

		if(last > 32 && last < 127) {
			info->kerning[last-32][c-32];
			ox += info->kerning[last-32][c-32] * pixelSize * fontScale * 0.5f;
		}

		//Account for the extra padding added to the bounding box to ensure that 
		//the SDF has enough room.
		f32 gx = (a->bbx - padding) * scaledRatio;
		//Because we pass the entire vertical area of the individual rendered 
		//glyphs to the final atlas, you don't need to align them the same way 
		//in Y. If you did decide to trim the glyphs, you would need to do
		//the same thing. I found this could lead to single pixel offset problems
		//(though, my math may have been wrong too), so I stuck to keeping the 
		//entire glyph rendered.

		//We need to scoot the first glyph on each line backward somewhat so that
		//the glyph is close to the passed in X position
		//For some reason, multiplying by 1.25 works
		if(newLine) {
			ox -= gx * 1.25;
			newLine = 0;
		}

		//hey, at least these units make sense
		exampleDrawRect(sdfSharpness, color
				x + ox + gx, y + oy, a->w * scaledRatio, a->h * scaledRatio,
				a->x + info->atlasX, a->y + info->atlasY, a->w, a->h);

		//but this one doesn't?
		ox += g->advance * heightRatio;
		last = c;
	}
}

//
//The SDF parameter in exampleDrawRect is, I don't know what the units should be,
//but numbers around 0.001 seem to work? (in the demo, I divide by 1000)
//
//In the original shader, it's replaced by fwidth.
//

/* Sample (OpenGL 2.0) fragment shader.
 *
 * This is essentially the same as the msdf one, but with the sdfSharpness term
 * passed in, rather than just with fwidth. 
 *
 */
#version 100

varying float fSdfSharpness;
varying vec2 fScale;
varying vec2 fUV;
varying vec4 fColor;

uniform float uPxRange;
uniform vec2 uInvTextureSize;
uniform vec4 uTint;
uniform sampler2D uTexture;

float median(float a, float b, float c)
{
	return max(min(a, b), min(max(a, b), c));
}

void main()
{
	vec2 msdfUnit = uPxRange * uInvTextureSize;
	vec4 sdfVal = texture2D(uTexture, fUV * uInvTextureSize);
	float sigDist = median(sdfVal.r, sdfVal.g, sdfVal.b) - 0.5;
	sigDist *= dot(msdfUnit, 0.5/vec2(fSdfSharpness));
	float opacity = clamp(sigDist + 0.5, 0.0, 1.0);
	gl_FragColor = vec4(1, 1, 1, opacity) * uTint * fColor;
}

#endif
