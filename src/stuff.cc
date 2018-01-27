
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <intrin.h>

typedef const char* string;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t usize;
typedef ptrdiff_t isize;
#define FNV64_Basis  14695981039346656037UL
#define FNV64_Prime  1099511628211UL

u64 hashString(string s)
{
	u64 hash = FNV64_Basis;
	while(*s != '\0') {
		hash ^= *s++;
		hash *= FNV64_Prime;
	}
	return hash;
}

void createdir(char* name)
{
	if(!PathFileExists(name)) {
		CreateDirectory(name, NULL);
	}
}

char* loadFile(char* filename)
{
	char* str = NULL;
	FILE* fp = fopen(filename, "rb");
	if(fp != NULL) {
		fseek(fp, 0L, SEEK_END);
		isize size = ftell(fp);
		rewind(fp);
		str = (char*)malloc(size + 1);
		fread(str, sizeof(char), size, fp);
		str[size] = '\0';
		fclose(fp);
	} else {
		fprintf(stderr, ">>> Could not open file %s\n", filename);
	}
	return str;
}

void writeFile(char* filename, void* data, isize size)
{
	FILE* fp = fopen(filename, "wb");
	if(fp != NULL) {
		fwrite(data, size, 1, fp);
		fclose(fp);
	} else {
		fprintf(stderr, ">>> Could not open file %s\n", filename);
	}
}


#define wplCopyMemory memcpy
void wplCopyMemoryBlock(void* dest, const void* source, 
		i32 sx, i32 sy, i32 sw, i32 sh, i32 stw, i32 sth,
		i32 dx, i32 dy, i32 dw, i32 dh,
		i32 size, i32 border)
{
	u8* dst = (u8*)dest;
	const u8* src = (u8*)source;
	for(isize i = 0; i < sh; ++i) {
		wplCopyMemory(
			dst + ((i+dy) * dw + dx) * size, 
			src + ((i+sy) * stw + sx) * size,
			sw * size);
	}

	if(border) {
		for(isize i = 0; i < sh; ++i) {
			wplCopyMemory(
					dst + ((i+dy) * dw + (dx-1)) * size, 
					dst + ((i+sy) * dw + sx) * size,
					1 * size);

			wplCopyMemory(
					dst + ((i+dy) * dw + (dx+sw)) * size, 
					src + ((i+sy) * dw + (sx+sw-1)) * size,
					1 * size);
		}

		wplCopyMemory(
			dst + ((dy-1) * (dw) + (dx-1)) * size, 
			dst + ((dy) * (dw) + (dx-1)) * size,
			(sw+2) * size);

		wplCopyMemory(
			dst + ((dy+sh) * (dw) + (dx-1)) * size, 
			dst + ((dy+sh-1) * (dw) + (dx-1)) * size,
			(sw+2) * size);
	}
}

union icvt
{
	unsigned int u;
	int i;
};
