#define printf printf_
#include <stdio.h> // vsprintf
#undef printf

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "gdi32.lib")

#include <windows.h>


#include <stdint.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;

#include <stdbool.h>
#include <stdlib.h>

// FORNOW: Sleep
#define assert(condition) \
    do { \
        if (!(condition)) { \
            printf("assert(" #condition "); failed // line %d of %s\n", __LINE__, __FILE__); \
            Sleep(2000); \
            char *ptr = NULL; *ptr = 0; \
        } \
    } while (0) 

int printf(const char *format, ...) {
    static char buffer[4096];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    DWORD length = (DWORD) strlen(buffer);
    DWORD written;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buffer, length, &written, NULL);
    assert(written == length);

    return(length);
}

float inverseLerp(float l, float u, float pos) {
  return ((float)pos - l) / (u - l);
}

float lerp(float l, float u, float t) {
  return (((u - l) * t) + l);
}

u64 wig_get_timestamp() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER ul = {.LowPart = ft.dwLowDateTime, .HighPart = ft.dwHighDateTime};
    return ((u64) (ul.QuadPart - 116444736000000000ULL)) / 10000;
}

#define TAU 6.28
f32 wig_sin(f32 turns) {
    return sinf(turns * TAU);
}

f32 wig_cos(f32 turns) {
    return cosf(turns * TAU);
}

f32 wig_atan2(f32 y, f32 x) {
    f32 angle_rad = atan2f(y, x);
    f32 turns = angle_rad / TAU;
    if (turns < 0.0f) {
        turns += 1.0f;
    }
    return turns;
}   