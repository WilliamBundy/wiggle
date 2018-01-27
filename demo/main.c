/* If you want to change the font used, just edit these
 * (you need to run the example in the upper folder)
 */
#define FontInfoFile "sspr.wfi"
#define GraphicsFile "game_atlas.rgba"

#include <Windows.h>
#include <intrin.h>
#include "textSample.h"
int _fltused = 1;

typedef unsigned char u8;
typedef short int i16;
typedef int i32;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef float f32;
typedef __int64 isize;
typedef __m128 vf128;
typedef union vf32x4 vf32x4;
union vf32x4
{
	vf128 v;
	f32 f[4];
};

#define ShuffleToByte(x, y, z, w) (((x)<<6) | ((y)<<4) | ((z)<<2) | w)
#define vfShuffle(a, x, y, z, w) _mm_shuffle_ps(a, a, ShuffleToByte(x, y, z, w))

static inline
float wabsf(f32 x) 
{
	union {
		f32 f;
		i32 i;
	} cc = {x};
	cc.i &= ~(1<<31);
	return cc.f;
}

u32 randSeed;
u32 badRand()
{
	randSeed = 65521 * randSeed + 12345;
	return randSeed;
}

void* alloc(isize size)
{
	return (void*)GlobalAlloc(GMEM_ZEROINIT, size);
}

#define WIGGLE_NO_TYPES
#include "../wiggle_types.h"

wFontInfo* loadFontInfo(char* filename)
{
	HANDLE file = CreateFile(filename, 
			GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	wFontInfo* font = alloc(sizeof(wFontInfo));
	u32 size = 0;
	ReadFile(file, font, sizeof(wFontInfo), &size, NULL);
	if(size != sizeof(wFontInfo)) {
		CloseHandle(file);
		GlobalFree(font);
		return NULL;
	}

	CloseHandle(file);
	return font;
}

#include "shaders.h"
#include "render.c"
wRenderGroup g;

#define KeyLeft 37
#define KeyUp 38
#define KeyRight 39
#define KeyDown 40
#define KeySpace 32
#define KeyReturn 13

int running = 1;
char keys[256];
void handleKey(int down, int key)
{
	keys[key] = down;
}

#include "game.c"

static
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message) {
		case WM_QUIT:
		case WM_CLOSE:
			running = 0;
			break;

		case WM_KEYDOWN:
			if(!(lparam & (1 << 30)))  {
				handleKey(1, (int)wparam);
			}
			break;

		case WM_KEYUP:
				handleKey(0, (int)wparam);
			break;

		default:
			return DefWindowProcA(window, message, wparam, lparam);
	}

	return 0;
}


void initGL(HDC windowDC);
void TextInvadersMain()
{
	{ //seed RNG using QPC
		LARGE_INTEGER i;
		QueryPerformanceCounter(&i);
		randSeed = i.LowPart;
	}

	HANDLE module = GetModuleHandle(NULL);
	WNDCLASSA windowClass = {0};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = windowCallback;
	windowClass.hInstance = module;
	windowClass.lpszClassName = "TextInvadersWindowClass";

	if(!RegisterClassA(&windowClass)) {
		goto Failure;
	}

	//We want to create a window locked to 640x480
	HWND window = CreateWindowExA(0,
			windowClass.lpszClassName,
			"Text Invaders", 
			WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,
			CW_USEDEFAULT, CW_USEDEFAULT,
			640, 480,
			0, 0, windowClass.hInstance, 0);

	HDC windowDC = GetDC(window);
	initGL(windowDC);
	
	if(ShowWindow(window, SW_SHOWNORMAL) != 0) {
		goto Failure;
	}

	glClearColor(0, 0, 0, 1);
	font = loadFontInfo(FontInfoFile);

	wShader s;
	wInitDefaultShader(GLES2_vert, GLES2_frag, &s);

	wTexture* t = wLoadTexture(GraphicsFile);
	wUploadTexture(t);

	wGroupInit(&g, 4096, &s, t);

	init();
	while(running) {
		MSG message;
		while(PeekMessage(&message, window, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		//Check game.c for details                   _       _
		//Effectively locked to vsync, assumes 60fps  \(._.)/
		update();

		wGroupDraw(640, 480, &g);
		SwapBuffers(windowDC);
	}

Failure:
	ExitProcess(0);
}

#define GL_TRUE 1
#define GL_FALSE 0
#define wglProcList \
	wglProc(HGLRC WINAPI, wglCreateContextAttribsARB, HDC hDC, HGLRC hShareContext, const int* attribList) \
	wglProc(BOOL WINAPI, wglSwapIntervalEXT, int interval) \
	wglProc(BOOL WINAPI, wglChoosePixelFormatARB, HDC hDC, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats) \

#define wglProc(ret, name, ...) typedef ret name##Proc(__VA_ARGS__); static name##Proc *name;
wglProcList
#undef wglProc

#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x2
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_FLAGS_ARB 0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126

static void* loadGlProc(char* name, HMODULE userdata)
{
	HMODULE gldll = userdata;
	void* p = (void*)wglGetProcAddress(name);
	long long int err = (long long int)p;
	if(err == 0 || err == 1 || err == 2 || err == 3 || err == -1) {
		p = (void*)GetProcAddress(gldll, name);
    }
	return p;
}

void initGL(HDC windowDC)
{
	// Load the few wgl*** extensions we need.
	{
		WNDCLASSA windowClass = {0};
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = DefWindowProcA;
		windowClass.hInstance = GetModuleHandle(NULL);
		windowClass.lpszClassName = "TraceteroidsWglClassName";
		if(!RegisterClassA(&windowClass)) {
			return;
		}

		HWND window = CreateWindowExA(0,
				windowClass.lpszClassName,
				"wbPlatform wgl loader", 
				0,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				0, 0, GetModuleHandle(NULL), 0);

		if(!window) {
			return;
		}

		HDC windowDC = GetDC(window);
		PIXELFORMATDESCRIPTOR pfd = {0};
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		pfd.cColorBits = 24;
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;

		int pfi = ChoosePixelFormat(windowDC, &pfd);
		PIXELFORMATDESCRIPTOR spfd;
		DescribePixelFormat(windowDC, pfi, sizeof(spfd), &spfd);
		if(!SetPixelFormat(windowDC, pfi, &spfd)) {
			return;
		}

		HGLRC glctx = wglCreateContext(windowDC);
		if(!wglMakeCurrent(windowDC, glctx)) {
			return;
		}

#define wglProc(ret, name, ...) name = (name##Proc *)wglGetProcAddress(#name); 
wglProcList;
#undef wglProc

		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(glctx);
		ReleaseDC(window, windowDC);
		DestroyWindow(window);
	}

	const int pixelAttribs[] =  {
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_COLOR_BITS_ARB, 32,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		0
	};

	int pfi;
	unsigned int count;
	if(!wglChoosePixelFormatARB(windowDC, pixelAttribs, 0, 1, &pfi, &count)) {
		return;
	}

	PIXELFORMATDESCRIPTOR spfd;
	DescribePixelFormat(windowDC, pfi, sizeof(spfd), &spfd);
	if(!SetPixelFormat(windowDC, pfi, &spfd)) {
		return;
	}

	const int glAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 2,
		WGL_CONTEXT_MINOR_VERSION_ARB, 1,
		WGL_CONTEXT_FLAGS_ARB,
			0,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0
	};
	int ver = 0;

	HGLRC glctx = wglCreateContextAttribsARB(windowDC, 0, glAttribs);
	if(!glctx) {
		return;
	}

	if(!wglMakeCurrent(windowDC, glctx)) {
		return;
	}

	wglSwapIntervalEXT(1);


	void* userdata = LoadLibraryA("opengl32.dll");
#define GLproc(ret, name, ...) name = (name##Proc *)loadGlProc(#name, userdata); 
	GL_ProcList
#undef GLproc
}
