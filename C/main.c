#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <time.h>
#include "basics.c"
#include "serial.c"
#include "pipe.c"

HANDLE teensyHandle;
HANDLE unityHandle;
bool unityIsConnected;

//globals for physics things in c (units in C SCALE)
f32 simAngle;
rect simStick;
rect simWall;

//TODO global origin no work for simpinball i'm crashing out
vec2 global_origin = {250, 250};
pinball simPinball = {{250, 250}, 10, 10.0f / 1000.0f};

int unityNumFloatsToRead = 5;
int unityNumFloatsToWrite = 3;
int numBytesInFloat = 4;

u64 timestamp;

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    f32 angle;
    f32 ball_position_x;
    f32 ball_position_z;
} angleAndBall;

typedef struct
{
    f32 ball_position_x;
    f32 ball_position_z;
    vec3 hand_position;

} unityPositions;

////////////////////////////////////////////////////////////////////////////////

unityPositions unity_read_current_virtual_positions()
{
    static unityPositions result;
    // reads hand and ball position
    while (pipe_available(unityHandle) >= unityNumFloatsToRead * numBytesInFloat)
    {
        pipe_read_n_bytes(unityHandle, unityNumFloatsToRead * numBytesInFloat, &result);
    }
    return (result);
}

void unity_write_target_virtual_data(angleAndBall targetUnityData)
{
    pipe_write_n_bytes(unityHandle, unityNumFloatsToWrite * numBytesInFloat, &targetUnityData);
}

////////////////////////////////////////////////////////////////////////////////

f32 teensy_read_current_physical_angle()
{
    static f32 result;
    while (serial_available(teensyHandle) >= numBytesInFloat)
    {
        serial_read_n_bytes(teensyHandle, numBytesInFloat, &result);
    }
    return (result);
}

void teensy_write_target_physical_angle(f32 target_physical_angle)
{
    serial_write_n_bytes(teensyHandle, numBytesInFloat, &target_physical_angle);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
    f32 current_physical_angle;
    vec3 current_virtual_hand_position;
    f32 current_virtual_ball_position_x;
    f32 current_virtual_ball_position_z;
} OptInput;

typedef struct
{
    f32 target_physical_angle;
    f32 target_virtual_angle;
    f32 target_ball_position_x;
    f32 target_ball_position_z;
} OptOutput;

// also sets globals
OptInput opt_read_input()
{
    OptInput result = {0};
    f32 read_physical_angle = teensy_read_current_physical_angle();
    unityPositions read_virtual_positions = unity_read_current_virtual_positions();
    result.current_virtual_hand_position = read_virtual_positions.hand_position;
    result.current_virtual_ball_position_x = read_virtual_positions.ball_position_x;
    result.current_virtual_ball_position_z = read_virtual_positions.ball_position_z;

    result.current_physical_angle = read_physical_angle; // was -1
    simAngle = turns_to_angles(read_physical_angle);
    simPinball.center.x = global_origin.x + unity_to_c_scale(read_virtual_positions.ball_position_x);
    simPinball.center.y = global_origin.y + unity_to_c_scale(read_virtual_positions.ball_position_z);

    return (result);
}

void opt_write_output(OptOutput output, OptInput input)
{
    angleAndBall unity_target_data = {0};
    unity_target_data.angle = input.current_physical_angle;
    unity_target_data.ball_position_x = simPinball.center.x;
    unity_target_data.ball_position_z = simPinball.center.y;
    unity_write_target_virtual_data(unity_target_data);
    teensy_write_target_physical_angle(output.target_physical_angle); // was -1
}

// NOTE: all angles in turns
// map angle_01 from domain [0, 1] to be as close as possible to reference_angle
f32 remap_angle(f32 angle_01, f32 reference_angle)
{
    f32 floored = floor(reference_angle);
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

OptOutput opt_optimize(OptInput input)
{
    OptOutput result = {0};
    f32 angle_01 = wig_atan2(input.current_virtual_hand_position.z, input.current_virtual_hand_position.x);
    f32 angle = remap_angle(angle_01, input.current_physical_angle);
    result.target_virtual_angle = angle;
    result.target_physical_angle = angle;
    return (result);
}

void opt_wrapper() 
{
    OptInput input = opt_read_input();
    OptOutput output = opt_optimize(input);
    opt_write_output(output, input);
}

////////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;
    if (0)
    {
    }
    else if (msg == WM_KEYDOWN)
    {
        if (0)
        {
        }
        else if (wParam == 'Q' || wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if (wParam == 'T')
        {
            serial_write_byte(teensyHandle, 'A');
        }
        else if (wParam == 'U' && unityIsConnected)
        {
            serial_write_byte(unityHandle, 'A');
        }
    }
    else if (msg == WM_PAINT)
    {
        /*

        p_1----------p_2
         |            |
         |            |
        p_4----------p_3

        */
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HPEN pen = CreatePen(PS_SOLID, 3, RGB(255, 128, 0));
        SelectObject(hdc, pen);

        //draw stick
        MoveToEx(hdc, simStick.p1.x, simStick.p1.y, NULL);
        LineTo(hdc, simStick.p2.x, simStick.p2.y);
        LineTo(hdc, simStick.p3.x, simStick.p3.y);
        LineTo(hdc, simStick.p4.x, simStick.p4.y);
        LineTo(hdc, simStick.p1.x, simStick.p1.y);

        //draw circle
        MoveToEx(hdc, simPinball.center.x, simPinball.center.y, NULL);
        Ellipse(hdc, simPinball.center.x - simPinball.radius, simPinball.center.y - simPinball.radius, simPinball.center.x + simPinball.radius, simPinball.center.y + simPinball.radius);

        DeleteObject(pen);
        EndPaint(hwnd, &ps);
    }
    else if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
    }
    else
    {
        result = DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return (result);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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

    char *token = strtok(lpCmdLine, " ");
    while (token != NULL)
    {
        if (strcmp(token, "--no-unity") == 0)
        {
            unityIsConnected = false;
            printf("Unity Deactivated");
        }
        token = strtok(NULL, " ");
    }

    teensyHandle = serial_open("COM11", 115200);
    unityHandle = pipe_create("UnityPipe");

    angleAndBall zero_values = {0};
    unity_write_target_virtual_data(zero_values);
    teensy_write_target_physical_angle(zero_values.angle);

    u64 timestamp = 0;
    MSG msg;
    int count = 0;

    while (1)
    {
        u64 new_timestamp = wig_get_timestamp();

        if ((new_timestamp - timestamp) > (30))
        {
            //update stick position
            simStick.p1 = rotate_coordinate(global_origin, (vec2){-10, -10}, simAngle);
            simStick.p2 = rotate_coordinate(global_origin, (vec2){60, -10}, simAngle);
            simStick.p3 = rotate_coordinate(global_origin, (vec2){60, 10}, simAngle);
            simStick.p4 = rotate_coordinate(global_origin, (vec2){-10, 10}, simAngle);
            //simStick = {p1, p2, p3, p4, simAngle};

            // TODO change collision function to calculate min and max, so you can pass any two points of the rect
            bool is_colliding = circle_rectangle_collides(rotate_coordinate(global_origin, simPinball.center, simAngle),
                                                        simPinball.radius,
                                                        rotate_coordinate(global_origin, simStick.p4, simAngle),
                                                        rotate_coordinate(global_origin, simStick.p2, simAngle));
            if (is_colliding) {
                simPinball.velocity *= -1;
                printf("collision at: %lld\n", new_timestamp);
            }
            
            //update ball position
            simPinball.center.y += 30 * simPinball.velocity;

            timestamp = new_timestamp;
            InvalidateRect(hwnd, NULL, true);
        }

        while (!unityIsConnected)
        {
            unityIsConnected = pipe_attempt_to_connect(unityHandle);
            if (unityIsConnected)
                printf("[INFO] Connected to Unity via pipe.\n");
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        opt_wrapper();
    }

quit:

    return (0);
}
