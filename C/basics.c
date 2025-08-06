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

typedef struct {
    f32 x;
    f32 y;
    f32 z;
} vec3;

typedef union {
    struct {
        f32 x;
        f32 y;
    };
    f32 _[2];
} vec2;

/*

p_2----------p_1
 |            |
 |            |
p_3----------p_4

*/
typedef struct {
    vec2 p1;
    vec2 p2;
    vec2 p3;
    vec2 p4;

} rect;

typedef struct {
    vec2 center;
    f32 radius;
    vec2 velocity;
} pinball;

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

void draw_rect(rect rectangle, HDC hdc) {
    MoveToEx(hdc, (int) rectangle.p1.x, (int) rectangle.p1.y, NULL);
    LineTo(hdc, (int) rectangle.p2.x, (int) rectangle.p2.y);
    LineTo(hdc, (int) rectangle.p3.x, (int) rectangle.p3.y);
    LineTo(hdc, (int) rectangle.p4.x, (int) rectangle.p4.y);
    LineTo(hdc, (int) rectangle.p1.x, (int) rectangle.p1.y);
}

float unity_to_c_scale(float n) {
    return n * 100;
}

float c_to_unity_scale(float n) {
    return n / 100.0f;
}

float turns_to_angles(float n) {
    return n * 6.283f;
}

float angles_to_turns(float n) {
    return n / 6.283f;
}

bool circle_rectangle_collides(vec2 c, float radius, vec2 min, vec2 max) {
    for (int d = 0; d < 2; ++d) {
        assert(min._[d] <= max._[d]);
    }

    float test; {
        test = 0.0f;
        for (int d = 0; d < 2; ++d) {
            if (c._[d] < min._[d]) {
                float tmp = (min._[d] - c._[d]);
                test += (tmp * tmp);
            } else if (max._[d] < c._[d]) {
                float tmp = (c._[d] - max._[d]);
                test += (tmp * tmp);
            }
        }
    }
    float squared_radius = (radius * radius);
    return (test < squared_radius);
}



vec2 rotate(vec2 point, f32 angle) {
    f32 c = cosf(angle);
    f32 s = sinf(angle);
    vec2 result;
    result.x = point.x * c - point.y * s;
    result.y = point.x * s + point.y * c;
    return result;
}

vec2 rotate_about(vec2 origin, vec2 point, f32 angle) {
    f32 c = cosf(angle);
    f32 s = sinf(angle);
    vec2 q = { point.x - origin.x, point.y - origin.y };
    vec2 result;
    result.x = origin.x + q.x * c - q.y * s;
    result.y = origin.y + q.x * s + q.y * c;
    return result;
}

bool rotated_circle_rectangle_collides(f32 angle, vec2 c, f32 radius, vec2 min, vec2 max) {
    c = rotate(c, -angle);
    min = rotate(min, -angle);
    max = rotate(max, -angle);

    for (int d = 0; d < 2; d++) {
        if (max._[d] < min._[d]) {
            f32 tmp = min._[d];
            min._[d] = max._[d];
            max._[d] = tmp;
        }
    }

    return circle_rectangle_collides(c, radius, min, max);
}

// NOTE: all angles in turns
// map angle_01 from domain [0, 1] to be as close as possible to reference_angle
f32 remap_angle(f32 angle_01, f32 reference_angle)
{
    f32 floored = floorf(reference_angle);
    f32 remainder = reference_angle - floored;
    f32 delta = angle_01 - remainder;
    if (delta > 0.5f)
    {
        angle_01 -= 1.0f;
    }
    else if (delta < -0.5f)
    {
        angle_01 += 1.0f;
    }

    f32 result = floored + angle_01;
    static f32 prev_angle_01;
    static f32 prev_reference_angle;
    static f32 prev_result;
    prev_angle_01 = angle_01;
    prev_reference_angle = reference_angle;
    prev_result = result;
    return result;
}
/*
// Conserve Momentum and Energy
vec2 update_ball_and_stick_trajectory_on_collision(*velocity ball_vel, *velocity stick_vel) {

}
*/
u64 wig_get_timestamp() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    ULARGE_INTEGER ul = {.LowPart = ft.dwLowDateTime, .HighPart = ft.dwHighDateTime};
    return ((u64) (ul.QuadPart - 116444736000000000ULL)) / 10000;
}

#define TAU 6.28f
f32 wig_sin(f32 turns) {
    return sinf(turns * TAU);
}

f32 wig_cos(f32 turns) {
    return cosf(turns * TAU);
}

f32 wig_atan2(f32 z, f32 x) {
    f32 angle_rad = atan2f(z, x);
    f32 turns = angle_rad / TAU;
    if (turns < 0.0f) {
        turns += 1.0f;
    }
    return turns;
}   