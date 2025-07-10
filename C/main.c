#include "basics.c"
#include "serial.c"

HANDLE handle;
//HBRUSH brush;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LRESULT result = 0;
    if (0) {
    } else if (msg == WM_KEYDOWN) {
        if (0) {
        } else if (wParam == 'Q' || wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        } else if (wParam == ' ') {
            serial_write_byte(handle, 'A');
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

    handle = serial_open("COM11", 115200);
    int red;
    int green;
    int blue;

    // TODO (Students): receive byte from teensy (maybe when pressing a button?)
    //                  (could change window background color to show button has been received)
    // NOTE: Will need to solder up the teensy; will need to get bread board and button from the fun wagon
    // TODO (Students): get the knob going and swirl the background color through the rainbow of your dreams

    MSG msg;
    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (serial_num_bytes_ready_to_read(handle)) {
            while (serial_num_bytes_ready_to_read(handle)) {
                serial_read_byte(handle);
            }
            //brush = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
            //printf("%c", serial_read_byte(handle));
            HDC hdc = GetDC(hwnd);
            RECT rc; GetClientRect(hwnd, &rc); 
            red = rand() % 255;
            green = rand() % 255;
            blue = rand() % 255;
            printf("red: %d, green: %d, blue: %d\n", red, green, blue);
            HBRUSH brush = CreateSolidBrush(RGB(red, green, blue));
            FillRect(hdc, &rc, brush); 
            DeleteObject(brush); // Free the created brush: see note below!
        }

    }

quit:

    return(0);
}
