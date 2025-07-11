#include "basics.c"
#include "serial.c"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    if (0) {
    } else if (msg == WM_KEYDOWN) {
        if (0) {
        } else if (wParam == 'Q' || wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        } else if (wParam == ' ') {
            serial_write_byte('A');
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
    int count = 0;
    //brush = CreateSolidBrush(RGB(0, 0, 0));

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

    serial_open("COM11", 115200);
    u8 encoderPosition;

    MSG msg;
    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (serial_num_bytes_ready_to_read()) {
            //NOTE: input must be raw bit representation
            encoderPosition = serial_read_byte();
            printf("%d\n", encoderPosition);
            while (serial_num_bytes_ready_to_read(handle)) {
                serial_read_byte(handle);
            }

            HDC hdc = GetDC(hwnd);
            RECT rc; GetClientRect(hwnd, &rc); 
            HBRUSH brush = CreateSolidBrush(RGB(225, 150, encoderPosition));
            FillRect(hdc, &rc, brush); 
            DeleteObject(brush);
        }

    }

quit:

    return(0);
}
