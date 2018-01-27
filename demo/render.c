/* I ripped most of this code out of other projects, so if it isn't super-neat,
 * that's probably why. This (and some code in main.c too) is a pretty minimal
 * OpenGL loader; I didn't want to attach thousands of lines of GL procs that I 
 * won't use to a project this small. 
 *
 * As a renderer, it targets OpenGL 2 (and ES too). It probably isn't very
 * efficient, but it works well enough for this, and probably most 2D games.
 *
 * The magic happens in wGroupInit, wDrawGroup, and the attached shaders.h too
 * Basic idea:
 * 	- Compile and link shaders
 * 	- Upload a texture
 * 	- Allocate some big arrays
 * 	- Store sprites in one array
 * 	- every frame:
 * 		- convert all the sprites to vertices
 * 		- upload all the verts to OpenGL
 * 		- use glDrawElements to draw all the triangles
 *
 *
 *
 *
 *
 */



typedef struct wSprite wSprite;
typedef struct wVertex wVertex;
typedef struct wRenderGroup wRenderGroup;
typedef struct wShader wShader;
typedef struct wTexture wTexture;
typedef struct wSpriteList wSpriteList;

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_ARRAY_BUFFER 0x8892
#define GL_FLOAT 0x1406
#define GL_UNSIGNED_BYTE 0x1401
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_RGBA 0x1908
#define GL_LINEAR 0x2601
#define GL_TEXTURE_MIN_FILTER 0x2800
#define GL_TEXTURE_MAG_FILTER 0x2801
#define GL_BLEND 0x0BE2
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_STREAM_DRAW 0x88E0
#define GL_TRIANGLES 0x0004
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_SHORT 0x1403
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000


#define GL_ProcList \
	GLproc(void, glClearColor, f32 r, f32 g, f32 b, f32 a)\
	GLproc(void, glClear, int num)\
	GLproc(void, glActiveTexture, i32 texture)\
	GLproc(void, glAttachShader, u32 program, u32 shader)\
	GLproc(void, glBindAttribLocation, u32 program, u32 index, const char *name)\
	GLproc(void, glBindBuffer, i32 target, u32 buffer)\
	GLproc(void, glBindTexture, i32 target, u32 texture)\
	GLproc(void, glBindVertexArray, u32 array)\
	GLproc(void, glBlendFunc, i32 sfactor, i32 dfactor)\
	GLproc(void, glBufferData, i32 target, i32 size, const void *data, i32 usage)\
	GLproc(void, glCompileShader, u32 shader)\
	GLproc(u32, glCreateProgram)\
	GLproc(u32, glCreateShader, i32 type)\
	GLproc(void, glEnable, i32 cap)\
	GLproc(void, glEnableVertexAttribArray, u32 index)\
	GLproc(void, glGenBuffers, i32 n, u32 *buffers)\
	GLproc(void, glGenTextures, i32 n, u32 *textures)\
	GLproc(void, glGenVertexArrays, i32 n, u32 *arrays)\
	GLproc(i32, glGetError)\
	GLproc(void, glGetProgramInfoLog, u32 program, i32 bufSize, i32 *length, char *infoLog)\
	GLproc(void, glGetProgramiv, u32 program, i32 pname, int *params)\
	GLproc(void, glGetShaderInfoLog, u32 shader, i32 bufSize, i32 *length, char *infoLog)\
	GLproc(void, glGetShaderiv, u32 shader, i32 pname, int *params)\
	GLproc(int, glGetUniformLocation, u32 program, const char *name)\
	GLproc(void, glLinkProgram, u32 program)\
	GLproc(void, glShaderSource, u32 shader, i32 count, const char** string, const int *length)\
	GLproc(void, glTexImage2D, i32 target, int level, int internalformat, i32 width, i32 height, int border, int format, int type, const void *pixels)\
	GLproc(void, glTexParameteri, i32 target, i32 pname, int param)\
	GLproc(void, glUniform1f, int location, float v0)\
	GLproc(void, glUniform2f, int location, float v0, float v1)\
	GLproc(void, glUniform4f, int location, float v0, float v1, float v2, float v3)\
	GLproc(void, glUniformMatrix4fv, int location, i32 count, int transpose, const float *value)\
	GLproc(void, glUseProgram, u32 program)\
	GLproc(void, glGenerateMipmap, i32 target)\
	GLproc(void, glDrawElements, int mode, int count, int type, void* usage)\
	GLproc(void, glVertexAttribPointer, u32 index, int size, i32 type, boolean normalized, i32 stride, const void *pointer)


#define GLproc(ret, name, ...) typedef ret name##Proc(__VA_ARGS__); static name##Proc *name;
GL_ProcList
#undef GLproc

struct wSprite
{
	i32 flags;
	u32 color;
	f32 x, y, w, h, cx, cy;
	i16 tx, ty, tw, th;
	f32 sdf;
};

struct wVertex
{
	f32 kind;
	f32 x, y;
	f32 u, v;
	f32 sx, sy;
	u32 color;
};

struct wShader
{
	u32 vert, frag, program;

	i32 uTint;
	i32 uInvTextureSize;
	i32 uPxRange;
};

struct wTexture
{
	i32 w, h;
	u32* pixels;
	u32 glIndex;
};

struct wSpriteList
{
	i32 start, count;
	f32 l, b, r, t;
};

struct wRenderGroup
{
	wTexture* texture;
	wShader* shader;
	u32 vao, vbo;

	i32 blank;
	i32 clearOnDraw;

	f32 dpi;
	f32 scale;
	f32 offsetX, offsetY;
	u32 tint;

	float sdfPxRange;

	wSprite* sprites;
	wVertex* verts;
	u16* indices;
	i32 count, capacity;
};

enum SpriteFlags
{
	Anchor_Center = 0,
	Anchor_TopLeft = 1,
	Anchor_TopCenter = 2,
	Anchor_TopRight = 3,
	Anchor_RightCenter = 4,
	Anchor_BottomRight = 5,
	Anchor_BottomCenter = 6,
	Anchor_BottomLeft = 7,
	Anchor_LeftCenter = 8,
	Sprite_Hidden = 1<<4,
	Sprite_NoTexture = 1<<5,
	Sprite_RotateCW = 1<<6,
	Sprite_RotateCCW = 1<<7,
	Sprite_FlipHoriz = 1<<8,
	Sprite_FlipVert = 1<<9,
	Sprite_NoAA = 1<<10,
	Sprite_MSDF = 1<<11,
};

wTexture* wLoadTexture(const char* filename)
{
	HANDLE file = CreateFile(filename, 
			GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	u32 buf[4];
	u32 size = 0;
	i32 ee = ReadFile(file, buf, 16, &size, NULL);

	wTexture* texture = alloc(sizeof(wTexture));
	texture->w = buf[2];
	texture->h = buf[3];
	isize pixelsSize = texture->w * texture->h * sizeof(u32);
	texture->pixels = alloc(pixelsSize);
	ee = ReadFile(file, texture->pixels, pixelsSize, &size, NULL);
	CloseHandle(file);
	return texture;
}

void wUploadTexture(wTexture* texture)
{
	glGenTextures(1, &texture->glIndex);
	glBindTexture(GL_TEXTURE_2D, texture->glIndex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
			texture->w, texture->h, 0, 
			GL_RGBA, GL_UNSIGNED_BYTE, 
			texture->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static
void wInitDefaultShader(const char* vertShader, const char* fragShader, wShader* shader)
{
	u32 vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertShader, NULL);
	glCompileShader(vert);
	u32 frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragShader, NULL);
	glCompileShader(frag);
	shader->vert = vert;
	shader->frag = frag;
	shader->program = glCreateProgram();

	glAttachShader(shader->program, vert);
	glBindAttribLocation(shader->program, 0, "vKind");
	glBindAttribLocation(shader->program, 1, "vPos");
	glBindAttribLocation(shader->program, 2, "vUV");
	glBindAttribLocation(shader->program, 3, "vScale");
	glBindAttribLocation(shader->program, 4, "vColor");
	glAttachShader(shader->program, frag);
	glLinkProgram(shader->program);

	glUseProgram(shader->program);
	shader->uInvTextureSize = glGetUniformLocation(
			shader->program, "uInvTextureSize");
	shader->uTint = glGetUniformLocation(shader->program, "uTint");
	shader->uPxRange = glGetUniformLocation(shader->program, "uPxRange");
}

void wInitSprite(wSprite* s)
{
	s->flags = 0;
	s->color = 0xFFFFFFFF;
	s->x = 0;
	s->y = 0;
	s->w = 0;
	s->h = 0;
	s->tx = 0;
	s->ty = 0;
	s->tw = 0;
	s->th = 0;
	s->sdf = 1.0f;
}

wSprite* wGroupAddRaw(
		wRenderGroup* group,
		i32 flags,
		u32 color,
		f32 x, f32 y,
		f32 w, f32 h,
		i16 tx, i16 ty, i16 tw, i16 th)
{
	wSprite s;
	wInitSprite(&s);
	s.flags = flags;
	s.color = color;
	s.x = x;
	s.y = y;
	s.w = w;
	s.h = h;
	s.cx = 0;
	s.cy = 0;
	s.tx = tx;
	s.ty = ty;
	s.tw = tw;
	s.th = th;
	group->sprites[group->count++] = s;
	return group->sprites + group->count - 1;
}


wSprite* wGetSprite(wRenderGroup* group)
{
	wSprite* s = group->sprites + group->count++;
	wInitSprite(s);
	return s;
}

void wGroupInit(wRenderGroup* group, i32 cap, wShader* shader, wTexture* texture)
{
	group->dpi = 72.0f;
	group->scale = 1;
	group->clearOnDraw = 1;
	group->offsetX = 0;
	group->offsetY = 0;
	group->tint = 0xFFFFFFFF;

	group->sdfPxRange = 8.0f;
	group->texture = texture;
	group->shader = shader;

	if(!texture->glIndex) {
		wUploadTexture(texture);
	}

	group->capacity = cap;
	group->sprites = alloc(sizeof(wSprite) * group->capacity);
	group->verts = alloc(sizeof(wVertex) * 4 * group->capacity);
	group->indices = alloc(sizeof(u16) * 6 * group->capacity);
	group->count = 0;

	glGenBuffers(1, &group->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, group->vbo);

	i32 i = 0;
	i32 stride = sizeof(wVertex);
	#define primMember(name) (void*)offsetof(wVertex, name)

	glVertexAttribPointer(i, 1, GL_FLOAT, 0, stride, primMember(kind));
	glEnableVertexAttribArray(i);
	i++;

	glVertexAttribPointer(i, 2, GL_FLOAT, 0, stride, primMember(x));
	glEnableVertexAttribArray(i);
	i++;

	glVertexAttribPointer(i, 2, GL_FLOAT, 0, stride, primMember(u));
	glEnableVertexAttribArray(i);
	i++;

	glVertexAttribPointer(i, 2, GL_FLOAT, 0, stride, primMember(sx));
	glEnableVertexAttribArray(i);
	i++;

	glVertexAttribPointer(i, 4, GL_UNSIGNED_BYTE, 1, stride, primMember(color));
	glEnableVertexAttribArray(i);
	i++;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

float SoffsetX[] = {0.0, 0.5, 0.0, -0.5, -0.5, -0.5,  0.0,  0.5, 0.5};
float SoffsetY[] = {0.0, 0.5, 0.5,  0.5,  0.0, -0.5, -0.5, -0.5, 0.0};

static
void groupProcessSprites(f32 width, f32 height, wRenderGroup* group)
{
	vf128 groupScale = _mm_set_ps1(group->scale);
	vf128 offsetXs = _mm_set_ps1(group->offsetX);
	vf128 offsetYs = _mm_set_ps1(group->offsetY);
	vf128 viewportXs = _mm_set_ps1(1.0f / width);
	vf128 viewportYs = _mm_set_ps1(1.0f / height);
	for(isize i = 0; i < group->count; ++i) {
		wSprite* s = group->sprites + i;
		if(s->flags & Sprite_Hidden) continue;
		isize i4 = i * 4;
		wVertex* p = group->verts + i4;
		u16* indexes = group->indices + (i*6);
		indexes[0] = i4 + 0;
		indexes[1] = i4 + 1;
		indexes[2] = i4 + 2;
		indexes[3] = i4 + 1;
		indexes[4] = i4 + 2;
		indexes[5] = i4 + 3;

		f32 uvrect[4];
		uvrect[0] = (f32)s->tx;
		uvrect[1] = (f32)s->ty;
		uvrect[2] = (f32)(s->tx + s->tw); 
		uvrect[3] = (f32)(s->ty + s->th);

		int f = s->flags & 0xf;
		vf128 xs = _mm_add_ps(_mm_set_ps(-0.5, -0.5, 0.5, 0.5),
				_mm_set1_ps(SoffsetX[f]));
		vf128 ys = _mm_add_ps(_mm_set_ps(0.5, -0.5, 0.5, -0.5),
				_mm_set1_ps(SoffsetY[f]));
		vf128 uvxs = _mm_set_ps(uvrect[0], uvrect[0], uvrect[2], uvrect[2]);
		vf128 uvys = _mm_set_ps(uvrect[3], uvrect[1], uvrect[3], uvrect[1]);

		if(s->flags & Sprite_FlipHoriz) {
			uvxs = vfShuffle(uvxs, 2, 3, 0, 1);
		}

		if(s->flags & Sprite_FlipVert) {
			uvys = vfShuffle(uvys, 2, 3, 0, 1);
		}

		f32 scaleX = s->w;
		f32 scaleY = s->h;
		if(s->flags & Sprite_RotateCW) {
			uvxs = vfShuffle(uvxs, 3, 1, 2, 0);
			uvys = vfShuffle(uvys, 3, 1, 2, 0);
			scaleX = s->h;
			scaleY = s->w;
		}

		if(s->flags & Sprite_RotateCCW) {
			uvxs = vfShuffle(uvxs, 2, 0, 3, 1);
			uvys = vfShuffle(uvys, 2, 0, 3, 1);
			scaleX = s->h;
			scaleY = s->w;
		}

		{
			vf128 scaleXs = _mm_set_ps1(scaleX * group->scale);
			vf128 scaleYs = _mm_set_ps1(scaleY * group->scale);
			xs = _mm_mul_ps(xs, scaleXs);
			ys = _mm_mul_ps(ys, scaleYs);
		}

		{
			vf128 pxs = _mm_set_ps1(s->x);
			pxs = _mm_mul_ps(pxs, groupScale);
			xs = _mm_add_ps(xs, pxs);
			xs = _mm_sub_ps(xs, offsetXs);

			vf128 pys = _mm_set_ps1(s->y);
			pys = _mm_mul_ps(pys, groupScale);
			ys = _mm_add_ps(ys, pys);
			ys = _mm_sub_ps(ys, offsetYs);
		}

		/* Normalize the position to -1, 1 based on the window size
		 * Essentially the orthographic matrix transform, flattened
		 * pos * vec2(2, -2) / viewportWH - vec2(1, -1)
		 */
		{
			vf128 number = _mm_set_ps1(2);
			xs = _mm_mul_ps(xs, number);
			xs = _mm_mul_ps(xs, viewportXs);
			number = _mm_set_ps1(1);
			xs = _mm_sub_ps(xs, number);

			number = _mm_set_ps1(-2);
			ys = _mm_mul_ps(ys, number);
			ys = _mm_mul_ps(ys, viewportYs);
			number = _mm_set_ps1(-1);
			ys = _mm_sub_ps(ys, number);
		}

		vf32x4 x = {xs}, y = {ys}, uvx = {uvxs}, uvy = {uvys};
		f32 ssx = x.f[2] - x.f[0];
		ssx /= s->tw;
		f32 ssy = y.f[1] - y.f[0];
		ssy /= s->th;
		for(isize j = 0; j < 4; ++j) {
			p[j].x = x.f[j];
			p[j].y = y.f[j];
			p[j].u = uvx.f[j];
			p[j].v = uvy.f[j];
			p[j].color = s->color; 
			p[j].sx = ssx;
			p[j].sy = ssy;
			if(s->flags & Sprite_MSDF) {
				p[j].kind = s->sdf;
			} else if(s->flags & Sprite_NoTexture) {
				p[j].kind = 40.0f;
			} else if(s->flags & Sprite_NoAA) {
				p[j].kind = 11.0f;
			} else {
				p[j].kind = 16.0f;
			}
		}
	}
}

void wGroupDraw(i32 width, i32 height, wRenderGroup* group)
{
	if(group->count == 0) return;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	wShader* shader = group->shader;
	glUseProgram(shader->program);

	glUniform2f(shader->uInvTextureSize,
			1.0f / (f32)group->texture->w, 
			1.0f / (f32)group->texture->h);
	glUniform4f(shader->uTint, 
			(f32)(group->tint & 0xFF) / 255.0f, 
			(f32)((group->tint >> 8) & 0xFF) / 255.0f,
			(f32)((group->tint >> 16) & 0xFF) / 255.0f,
			(f32)((group->tint >> 24) & 0xFF) / 255.0f);
	glUniform1f(shader->uPxRange, group->sdfPxRange);

	glBindTexture(GL_TEXTURE_2D, group->texture->glIndex);

	groupProcessSprites(width, height, group);
	glBindBuffer(GL_ARRAY_BUFFER, group->vbo);
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(wVertex) * 4 * group->count,
			group->verts,
			GL_STREAM_DRAW);

	glDrawElements(GL_TRIANGLES,
			group->count * 6,
			GL_UNSIGNED_SHORT,
			group->indices);
		
	if(group->clearOnDraw) {
		group->count = 0;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


int badstrlen(const char* s)
{
	int i = 0;
	while(*s++) i++;
	return i;
}

wSpriteList wDrawText(
		wRenderGroup* group, wFontInfo* info,
		f32 x, f32 y,
		const char* text, isize count,
		f32 pointSize, i32 flags, 
		u32 color, f32 sdfSharpness)
{
	f32 ox = 0.0f, oy = 0.0f;
	char last = 0;

	f32 pixelSize = (pointSize * group->dpi) / 72.0f;
	f32 padding = (f32)info->pxRange;
	f32 fontScale = info->scale;

	wGlyphImage* a = NULL;
	wGlyph* g = info->glyphs + ('A'-32);
	f32 glyphHeight = wabsf(g->t - g->b);
	f32 scaledHeight = glyphHeight * fontScale;
	f32 scaledRatio = pixelSize / scaledHeight;
	f32 heightRatio = pixelSize / glyphHeight;
	wSpriteList l;
	l.start = group->count;

	f32 maxX = 0;
	f32 widthP = 0;

	i32 newLine = 1;
	for(isize i = 0; i < count; ++i) {
		char c = text[i];
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

		f32 gx = (a->bbx - padding) * scaledRatio;
		if(newLine) {
			if(i == 0) widthP += gx;
			ox -= gx * 1.25;
			newLine = 0;
		}
		wSprite* s = wGroupAddRaw(group, flags | Sprite_MSDF, color,
				x + ox + gx, y + oy,
				a->w * scaledRatio, a->h * scaledRatio,
				a->x + info->atlasX, a->y + info->atlasY,
				a->w, a->h);
		s->sdf = sdfSharpness;
		ox += g->advance * heightRatio;
		if(ox > maxX) maxX = ox;
		last = c;
	}
	g = info->glyphs + ('A'-32);
	a = info->images + ('A'-32);
	l.count = group->count - l.start;
	l.l = x;
	l.t = y + a->bby * scaledRatio;
	l.r = x + maxX + widthP + padding * scaledRatio * 0.5f;
	l.b = y + oy + a->h * scaledRatio;
	return l;
}

