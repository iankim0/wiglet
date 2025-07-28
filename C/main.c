#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <time.h>
#include "basics.c"
#include "serial.c"
#include "pipe.c"

HANDLE teensyHandle;
HANDLE unityHandle;
bool unityIsConnected;
f32 odrivePosition;

u64 timestamp;

////////////////////////////////////////////////////////////////////////////////

typedef struct {
    f32 x;
    f32 y;
    f32 z;
} vec3;

////////////////////////////////////////////////////////////////////////////////

vec3 unity_read_current_virtual_hand_position() {
    static vec3 result; // *
    while (pipe_available(unityHandle) >= 12) {
        pipe_read_n_bytes(unityHandle, 12, &result);
    }
    return(result);
}

void unity_write_target_virtual_angle(f32 target_virtual_angle) {
    pipe_write_n_bytes(unityHandle, 4, &target_virtual_angle);
}

////////////////////////////////////////////////////////////////////////////////

f32 teensy_read_current_physical_angle() {
    static f32 result; // *
    while (serial_available(teensyHandle) >= 4) {
        serial_read_n_bytes(teensyHandle, 4, &result);
    }
    return(result);
}

void teensy_write_target_physical_angle(f32 target_physical_angle) {
    serial_write_n_bytes(teensyHandle, 4, &target_physical_angle);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct {
    f32 current_physical_angle;
    vec3 current_virtual_hand_position;
} OptInput;

typedef struct {
    f32 target_physical_angle;
    f32 target_virtual_angle;
} OptOutput;

OptInput opt_read_input() {
    OptInput result = {0};
    f32 read_physical_angle = teensy_read_current_physical_angle();
    result.current_virtual_hand_position = unity_read_current_virtual_hand_position();
    result.current_physical_angle = -1 * read_physical_angle;
    odrivePosition = read_physical_angle;
    return(result);
}

void opt_write_output(OptOutput output, OptInput input) {
    unity_write_target_virtual_angle(input.current_physical_angle);
    teensy_write_target_physical_angle(-1 * output.target_physical_angle);
}

// NOTE: all angles in turns
// map angle_01 from domain [0, 1] to be as close as possible to reference_angle

f32 remap_angle(f32 angle_01, f32 reference_angle) {
    f32 floored = floor(reference_angle); 
    f32 remainder = reference_angle - floored; 
    f32 delta = angle_01 - remainder; 
    if (delta > 0.5f) {
        angle_01 -= 1.0f; 
    } else if (delta < -0.5f) {
        angle_01 += 1.0f;
    } 

    f32 result = floored + angle_01;
    static f32 prev_angle_01;
    static f32 prev_reference_angle;
    static f32 prev_result;
    if (abs(result - prev_result) > 0.1f) {
      printf("\n\n\n");
      printf("(%f %f) -> %f\n", prev_angle_01, prev_reference_angle, prev_result);
      printf("(%f %f) -> %f\n", angle_01, reference_angle, result);
    }
    prev_angle_01 = angle_01;
    prev_reference_angle = reference_angle;
    prev_result = result;
    return result;
}

OptOutput opt_optimize(OptInput input) {
    OptOutput result = {0};
    f32 angle_01 = wig_atan2(input.current_virtual_hand_position.z, input.current_virtual_hand_position.x);
    f32 angle = remap_angle(angle_01, input.current_physical_angle); 
    result.target_virtual_angle = angle;
    result.target_physical_angle = angle;
    return(result);
}

void opt_wrapper() {
    OptInput input = opt_read_input();
    OptOutput output = opt_optimize(input);
    opt_write_output(output, input);
}

////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    if (0) {
    } else if (msg == WM_KEYDOWN) {
        if (0) {
        } else if (wParam == 'Q' || wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        } else if (wParam == 'T') {
            serial_write_byte(teensyHandle, 'A');
        } else if (wParam == 'U' && unityIsConnected) {
            serial_write_byte(unityHandle, 'A');
        }
    } else if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HPEN pen = CreatePen(PS_SOLID, 5, RGB(102,203,214)); 
        SelectObject(hdc, pen);
        MoveToEx(hdc, 250, 250, NULL);

        f32 angle = odrivePosition * 6.283f;
        int xEnd = 250 + (int) (50 * cosf(angle));
        int yEnd = 250 - (int) (50 * sinf(angle));
        LineTo(hdc, xEnd, yEnd);

        DeleteObject(pen);
        EndPaint(hwnd, &ps);
    } else if (msg == WM_DESTROY) {
        PostQuitMessage(0);
    } else {
        result = DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return(result);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    AllocConsole();
    SetConsoleTitle("Console");

    HWND hwnd;
    { // windows_init()
        WNDCLASS wc = {0};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = "Window";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClass(&wc);

        hwnd = CreateWindow("Window", "Window", WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                NULL, NULL, hInstance, NULL);

        SetWindowPos(hwnd, 0, 0, 0, 500, 500, 0);
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    char* token = strtok(lpCmdLine, " ");
    while (token != NULL) {
        if (strcmp(token, "--no-unity") == 0) {
            unityIsConnected = false;
            printf("Unity Deactivated");
        }
        token = strtok(NULL, " ");
    }

    teensyHandle = serial_open("COM11", 115200);
    unityHandle = pipe_create("UnityPipe");

    unity_write_target_virtual_angle(0.0f);
    teensy_write_target_physical_angle(0.0f);

    u64 timestamp = 0;
    MSG msg;
    int count = 0;
    while (1) {
        u64 new_timestamp = wig_get_timestamp();
        if ((new_timestamp - timestamp) > (30)) {
            timestamp = new_timestamp;
            InvalidateRect(hwnd, NULL, true);
        }

        while (!unityIsConnected) {
            unityIsConnected = pipe_attempt_to_connect(unityHandle);
            if (unityIsConnected) printf("[INFO] Connected to Unity via pipe.\n");
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        opt_wrapper();

        // Old code:
        // if (serial_available(teensyHandle) >= 4) {
        //     while (serial_available(teensyHandle) >= 4) { // FORNOW
        //         serial_read_n_bytes(teensyHandle, 4, &odrivePosition);
        //     }

        //     u64 new_timestamp = wig_get_timestamp();
        //     if ((new_timestamp - timestamp) > (1000U / 110U)) { // FORNOW 110
        //         timestamp = new_timestamp;
        //         if (unityIsConnected) {
        //             pipe_write_n_bytes(unityHandle, 4, &odrivePosition);
        //         }
        //     }
            
        //     InvalidateRect(hwnd, NULL, true);
        // }

        // if (unityIsConnected) {
        //     if (pipe_available(unityHandle)) {
        //         u8 byte_from_Unity;
        //         while (pipe_available(unityHandle)) {
        //             //NOTE: input must be raw bit representation
        //             pipe_read_byte(unityHandle, &byte_from_Unity);
        //         }
        //         // TODO: Purge???
        //         pipe_write_n_bytes(teensyHandle, 1, & byte_from_Unity);
        //     }
        // }

    }

quit:

    return(0);
}
