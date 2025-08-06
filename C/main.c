#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <time.h>
#include "basics.c"
#include "serial.c"
#include "pipe.c"

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 1000
#define ORIGIN_X WINDOW_WIDTH / 2
#define ORIGIN_Y WINDOW_HEIGHT * 2 / 3

HANDLE teensyHandle;
HANDLE unityHandle;
bool unityIsConnected;
bool DEACTIVATE_UNITY;
bool impulseActivated;
int impulseCounter;

//globals for physics things in c (units in C SCALE)

vec2 global_origin = {ORIGIN_X, ORIGIN_Y};
f32 prevAngle; // in radians
//TODO simAngle name is ambiguous--since it's also physical angle
f32 simAngle;
rect simStick;
rect simWall = {{ORIGIN_X + 60, ORIGIN_Y - 10 - 400}, {ORIGIN_X, ORIGIN_Y - 10 - 400}, {ORIGIN_X, ORIGIN_Y + 10 - 400}, {ORIGIN_X + 60, ORIGIN_Y + 10 - 400}};

pinball simPinball = {{ORIGIN_X + 30, ORIGIN_Y - 300}, 10.0f, 0.005f};

int numBytesInFloat = 4;

//TODO FIX STRUCT VS. GLOBAL ISSUE (excess steps in reading/writing)
////////////////////////////////////////////////////////////////////////////////
int unityNumFloatsToRead = 3;
int unityNumFloatsToWrite = 3;

typedef struct
{
    f32 angle;
    f32 ball_position_x;
    f32 ball_position_z;
} dataToUnity;

typedef struct
{
    vec3 hand_position;

} dataFromUnity;

////////////////////////////////////////////////////////////////////////////////

dataFromUnity unity_read_current_virtual_positions()
{
    static dataFromUnity result;

    while (pipe_available(unityHandle) >= unityNumFloatsToRead * numBytesInFloat)
    {
        pipe_read_n_bytes(unityHandle, unityNumFloatsToRead * numBytesInFloat, &result);
    }
    return (result);
}

void unity_write_target_virtual_data(dataToUnity targetUnityData)
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
} OptInput;

typedef struct
{
    f32 target_physical_angle;
    f32 target_ball_position_x;
    f32 target_ball_position_z;
} OptOutput;

OptInput opt_read_input()
{
    OptInput result = {0};

    dataFromUnity read_virtual_positions = {0};
    if (!DEACTIVATE_UNITY) {
        unity_read_current_virtual_positions();
    }

    result.current_physical_angle = teensy_read_current_physical_angle();
    result.current_virtual_hand_position = read_virtual_positions.hand_position;
    return (result);
}

void opt_write_output(OptOutput output, OptInput input)
{
    dataToUnity unity_target_data = {0};
    unity_target_data.angle = input.current_physical_angle;
    unity_target_data.ball_position_x = output.target_ball_position_x;
    unity_target_data.ball_position_z = output.target_ball_position_z;
    unity_write_target_virtual_data(unity_target_data);
    teensy_write_target_physical_angle(output.target_physical_angle); // was -1
}

OptOutput opt_optimize()
{
    OptOutput result = {0};

    // f32 angle_01 = wig_atan2(input.current_virtual_hand_position.z, input.current_virtual_hand_position.x);
    // f32 angle = remap_angle(angle_01, input.current_physical_angle);

    f32 tmp = impulseCounter ? turns_to_angles(-0.05f) : 0;

    result.target_physical_angle = angles_to_turns(simAngle + tmp);
    result.target_ball_position_x = c_to_unity_scale(simPinball.center.x - global_origin.x);
    result.target_ball_position_z = -1 * c_to_unity_scale(simPinball.center.y - global_origin.y);
    return (result);
}

//updates c globals
void update_sim(OptInput input) {
    simAngle = turns_to_angles(-1 * input.current_physical_angle);

    

    bool is_colliding = rotated_circle_rectangle_collides(simAngle, simPinball.center, simPinball.radius, simStick.p2, simStick.p4);

    bool is_wall_colliding = rotated_circle_rectangle_collides(0.0f, simPinball.center, simPinball.radius, simWall.p2, simWall.p4);
    
    if (is_colliding && simPinball.velocity.y > 0) {
        impulseActivated = true;
        impulseCounter = 0;

        f32 angular_velocity = simAngle - prevAngle;

        //0.9 for damping, add velocity of stick to ball
        simPinball.velocity = (-1 * simPinball.velocity * 0.9f) - (angular_velocity * ((simStick.p1.x - simStick.p2.x) / 2.0f)) / 1000.0f;

    } else if (is_wall_colliding && simPinball.velocity < 0) {
        //0.5 for damping
       simPinball.velocity *= -1 * 0.5;
    } else if (simPinball.center.y > global_origin.y + 100) {
        simPinball.velocity = 0;
    }

    if (impulseActivated) {
        if (impulseCounter++ == 5000) {
            impulseActivated = false;
            impulseCounter = 0;
        }
    }

    simPinball.center.y += simPinball.velocity;
} 

void opt_wrapper() 
{
    OptInput input = opt_read_input();
    update_sim(input);                          //calculates c globals
    OptOutput output = opt_optimize();          //sets all values (preparing to send)
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
        else if (wParam == 'R')
        {

        }
        else if (!DEACTIVATE_UNITY && wParam == 'U' && unityIsConnected)
        {
            serial_write_byte(unityHandle, 'A');
        }
    }
    else if (msg == WM_PAINT)
    {

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HPEN pen = CreatePen(PS_SOLID, 3, RGB(255, 128, 0));
        SelectObject(hdc, pen);

        //draw stick
        draw_rect(simStick, hdc);
        draw_rect(simWall, hdc);

        //draw circle
        Ellipse(hdc, (int) (simPinball.center.x - simPinball.radius), (int) (simPinball.center.y - simPinball.radius), (int) (simPinball.center.x + simPinball.radius), (int) (simPinball.center.y + simPinball.radius));

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
                            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
                            NULL, NULL, hInstance, NULL);

        SetWindowPos(hwnd, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd); 
    }

    char *token = strtok(lpCmdLine, " ");
    while (token != NULL)
    {
        if (strcmp(token, "--no-unity") == 0)
        {
            DEACTIVATE_UNITY = true;
            printf("DEACTIVATE_UNITY");
        }
        token = strtok(NULL, " ");
    }

    teensyHandle = serial_open("COM11", 115200);
    unityHandle = pipe_create("UnityPipe");

    u64 timestamp = 0;
    MSG msg;

    while (1)
    {
        u64 new_timestamp = wig_get_timestamp();
        if ((new_timestamp - timestamp) > (30))
        {     
            timestamp = new_timestamp;
            InvalidateRect(hwnd, NULL, true);
            prevAngle = simAngle;

            //gravity
            simPinball.velocity += 10.0f / 5000.0f;
        }

        if (!DEACTIVATE_UNITY) {
            while (!unityIsConnected)
            {
                unityIsConnected = pipe_attempt_to_connect(unityHandle);
                if (unityIsConnected)
                    printf("[INFO] Connected to Unity via pipe.\n");
            }
        }

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                goto quit;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        //update stick position
        simStick.p1 = rotate_about(global_origin, (vec2){global_origin.x - 10,global_origin.y - 10}, simAngle);
        simStick.p2 = rotate_about(global_origin, (vec2){global_origin.x + 60, global_origin.y - 10}, simAngle);
        simStick.p3 = rotate_about(global_origin, (vec2){global_origin.x + 60,global_origin.y + 10}, simAngle);
        simStick.p4 = rotate_about(global_origin, (vec2){global_origin.x - 10, global_origin.y + 10}, simAngle);

        opt_wrapper();
    }

quit:

    return (0);
}
