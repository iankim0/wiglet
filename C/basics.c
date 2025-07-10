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

#include <stdbool.h>
#include <stdlib.h>

// FORNOW: Sleep
#define assert(condition) \
    do { \
        if (!(condition)) { \
            printf("assert(" #condition "); failed // line %d\n", __LINE__); \
            Sleep(1000); \
            char *ptr = NULL; *ptr = 0; \
        } \
    } while (0) 

int printf(const char *format, ...) {
    static char buffer[4096];
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);

    u32 length = strlen(buffer);
    u32 written;
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buffer, length, &written, NULL);
    assert(written == length);

    return(length);
}

