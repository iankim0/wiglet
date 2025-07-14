#include "basics.c"
#include "serial.c"
#include "pipe.c"

HANDLE teensyHandle;
HANDLE unityHandle;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    if (0) {
    } else if (msg == WM_KEYDOWN) {
        if (0) {
        } else if (wParam == 'Q' || wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        } else if (wParam == 'T') {
            serial_write_byte(teensyHandle, 'A');
        } else if (wParam == 'U') {
            serial_write_byte(unityHandle, 'A');
        }
    } else if (msg == WM_DESTROY) {
        PostQuitMessage(0);
    } else {
        result = DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return(result);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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

    teensyHandle = serial_open("COM11", 115200);
    unityHandle = pipe_open("UnityPipe");

    u8 encoderPosition;
    MSG msg;
    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (serial_num_bytes_ready_to_read(teensyHandle)) {
            //NOTE: input must be raw bit representation
            encoderPosition = serial_read_byte(teensyHandle);

            // TODO: Purge ???
            while (serial_num_bytes_ready_to_read(teensyHandle)) {
                serial_read_byte(teensyHandle);
            }

            HDC hdc = GetDC(hwnd);
            RECT rc; GetClientRect(hwnd, &rc); 
            HBRUSH brush = CreateSolidBrush(RGB(225, 150, encoderPosition));
            FillRect(hdc, &rc, brush); 
            DeleteObject(brush);
        }

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

quit:

    return(0);
}
