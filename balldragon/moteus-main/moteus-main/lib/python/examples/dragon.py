# MOTEUS COMMAND LINE
# TVIEW: python -m moteus_gui.tview
# STOP:  python -m moteus.moteus_tool --target 1 --stop
# ZERO:  python -m moteus.moteus_tool --target 1 --zero-offset

import asyncio
import moteus
import socket
import struct
import math
import time

# Motor 1: 0.00 - 0.50
# Motor 2: 0.01 - 0.563


host, port = "127.0.0.1", 25001 #"10.32.13.235", 25001
data = [0.0, 0.0]
dataReceived = [-2.0, 0.2, 0.0, 0.25]

TORQUE_CONSTANT = 0.25 # The amount of feedforward torque needed to compensate for gravity when the arm is parallel to the ground
RESTING_ANGLE = -0.21 # The true resting angle of the motor arm in radians

start_time = time.time()

def lerp(a, b, f):
    return a + f * (b - a)

def calculate_feedforward_torque(x: float) -> float:
    return math.cos(x*6.2832 + RESTING_ANGLE) * TORQUE_CONSTANT


c_btm = moteus.Controller(id=1)
c_top = moteus.Controller(id=2)

async def stop_all():
    """Cleanup function to stop the motors."""
    await c_btm.set_stop()
    await c_top.set_stop()


async def main():

    for i in range(0, 2400):
        await c_btm.set_position(position = 0.25, accel_limit = 1.5, velocity_limit = 0.75)

    await stop_all()

    print("Aligned\n")

    # SOCK_STREAM means TCP socket 
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.bind((host, port))
        sock.listen(1)
        conn, addr = sock.accept()
        print(f"Connected by {addr}")
    finally:
        sock.close()

    with conn:

        # Clear outstanding faults
        await stop_all()

        # Wherever the moteus is currently, that becomes position 0.0
        # Make sure the motor arm is on the table as far as it can go counterclockwise / to the left
        await c_top.set_output_exact(position = 0.0)

        current_feedforward_torque = 0.0;

        while True:

            top_state = await c_top.query()
            top_pos = top_state.values[moteus.Register.POSITION]
            btm_state = await c_btm.query()
            btm_pos = btm_state.values[moteus.Register.POSITION]
            data[0] = top_pos
            data[1] = btm_pos
            
            conn.sendall(struct.pack('<2f', *data))

            # Receive stuff back from C#:
            dataReceived = struct.unpack('<3f', conn.recv(12))

            print("%5d: [%6.4f, %6.4f]" % (int(time.time() - start_time), data[0], data[1]))
            #print("     [%2d, %6.4f, %6.4f]" % (dataReceived[0], dataReceived[1], dataReceived[2]))

            if (dataReceived[0] == 1.0): # targeted + floaty
                current_feedforward_torque = calculate_feedforward_torque(top_pos)
                await c_top.set_position(position = dataReceived[1],
                                     accel_limit = 1.5,
                                     velocity_limit = dataReceived[2],
                                     feedforward_torque = current_feedforward_torque,
                                     kp_scale=1.7,
                                     kd_scale=0.8)
                await c_btm.set_position(position = 0.25, accel_limit = 1.5, velocity_limit = 0.75)
            elif (dataReceived[0] == 2.0): # targeted + weighty
                current_feedforward_torque = lerp(current_feedforward_torque, -0.5 * calculate_feedforward_torque(top_pos), 0.2);
                await c_top.set_position(position=dataReceived[1],
                                     accel_limit=1.5,
                                     velocity_limit = dataReceived[2],
                                     feedforward_torque = current_feedforward_torque,
                                     kp_scale=1.7,
                                     kd_scale=0.8)
                await c_btm.set_position(position = 0.25, accel_limit = 1.5, velocity_limit = 0.75)
            elif (dataReceived[0] == 3.0): # idle flying (floaty)
                current_feedforward_torque = calculate_feedforward_torque(top_pos)
                await c_top.set_position(position=top_pos,
                                     accel_limit=1.5, 
                                     velocity_limit = dataReceived[2],
                                     feedforward_torque = current_feedforward_torque,
                                     kp_scale=1.7,
                                     kd_scale=0.8)
                await c_btm.set_position(position = btm_pos, accel_limit = 1.5, velocity_limit = 0.75)
            elif (dataReceived[0] == 0.0): # power motor off
                await stop_all()
            elif (dataReceived[0] == -1.0): # power motor off and quit program
                await stop_all()
                conn.close()
                quit()

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Program interrupted.")