#define _CRT_SECURE_NO_WARNINGS
#include "basics.c"
#include "serial.c"
#include "pipe.c"
#include <math.h>



HANDLE teensyHandle;
HANDLE unityHandle;
bool usingUnity = true; 
float angle;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    if (0) {
    } else if (msg == WM_KEYDOWN) {
        if (0) {
        } else if (wParam == 'Q' || wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        } else if (wParam == 'T') {
            serial_write_byte(teensyHandle, 'A');
        } else if (wParam == 'U' && usingUnity) {
            serial_write_byte(unityHandle, 'A');
        }
    } else if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HPEN pen = CreatePen(PS_SOLID, 5, RGB(102,203,214)); 
        SelectObject(hdc, pen);
        MoveToEx(hdc, 250, 250, NULL);

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
            usingUnity = false;
            printf("Unity Deactivated");
        }
        token = strtok(NULL, " ");
    }

    teensyHandle = serial_open("COM11", 115200);
    if (usingUnity) {
        unityHandle = pipe_open("UnityPipe");
    }

    u64 timestamp = 0;
    MSG msg;
    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (serial_num_bytes_ready_to_read(teensyHandle)) {
            //NOTE: input must be raw bit representation
            // FORNOW
            u8 odrivePosition = 0;
            while (serial_num_bytes_ready_to_read(teensyHandle)) {
                odrivePosition = serial_read_byte(teensyHandle);
            }

            u64 new_timestamp = basics_get_timestamp();
            if ((new_timestamp - timestamp) > (1000U / 30U)) {
                timestamp = new_timestamp;
                serial_write_byte(unityHandle, odrivePosition);
            }
            
            angle = lerp(0.0f, 6.28f, inverseLerp(0.0f, 100.0f, odrivePosition));
            InvalidateRect(hwnd, NULL, true);
        }

        if (usingUnity) {
            if (pipe_num_bytes_ready_to_read(unityHandle)) {
                //NOTE: input must be raw bit representation
                u8 byte_from_Unity = pipe_read_byte(unityHandle);

                // TODO: Purge???

                HDC hdc = GetDC(hwnd);
                RECT rc; GetClientRect(hwnd, &rc); 
                HBRUSH brush = CreateSolidBrush(RGB(byte_from_Unity, 0, 0));
                FillRect(hdc, &rc, brush); 
                DeleteObject(brush);
            }
        }

    }

quit:

    return(0);
}
