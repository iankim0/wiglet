# MOTEUS COMMAND LINE
# TVIEW: python -m moteus_gui.tview
# STOP:  python -m moteus.moteus_tool --target 1 --stop
# ZERO:  python -m moteus.moteus_tool --target 1 --zero-offset

import asyncio
import moteus
import struct
import math
import time
import atexit
import sys
import threading

TORQUE_CONSTANT = 0.25  # The amount of feedforward torque needed to compensate for gravity when the arm is parallel to the ground
RESTING_ANGLE = -0.21  # The true resting angle of the motor arm in radians

def lerp(a, b, f):
    return a + f * (b - a)

def calculate_feedforward_torque(x: float) -> float:
    return math.cos(x * 6.2832 + RESTING_ANGLE) * TORQUE_CONSTANT

# Construct controllers at ids 1 and 2
c1 = moteus.Controller(id=1)
c2 = moteus.Controller(id=2)

async def cleanup():
    """Cleanup function to stop the motors."""
    print("Stopping motors...")
    await c1.set_stop()
    await c2.set_stop()

def get_user_input(input_queue):
    """Runs in a separate thread to get user input."""
    while True:
        try:
            user_input = input("Enter a new state value (integer): ")
            input_queue.put_nowait(int(user_input))  # Convert input to int and enqueue it
        except ValueError:
            print("Invalid input. Please enter an integer.")

async def main():
    # Register cleanup function
    atexit.register(lambda: asyncio.run(cleanup()))

    # Queue to share user input
    input_queue = asyncio.Queue()

    # Start a separate thread for user input
    input_thread = threading.Thread(target=get_user_input, args=(input_queue,), daemon=True)
    input_thread.start()

    # Clear outstanding faults
    await cleanup()

    start_time = time.time()

    # Wherever the moteus is currently, that becomes position 0.0
    # Make sure the motor arm is on the table as far as it can go counterclockwise / to the left

    # await c1.set_output_exact(position=0.0)
    # await c2.set_output_exact(position=0.0)

    target_value = 0  # Default target value

    while True:
        # Check for new user input
        if not input_queue.empty():
            target_value = await input_queue.get()  # Get the latest user input
            if target_value != 0:
                start_time = time.time()

        current_time = time.time() - start_time

        # Query motor states
        c1_state = await c1.query()
        c2_state = await c2.query()
        c1_position = c1_state.values[moteus.Register.POSITION]
        c2_position = c2_state.values[moteus.Register.POSITION]

        # Calculate feedforward torque
        current_feedforward_torque = calculate_feedforward_torque(c2_position)

        # Use the target_value in some way to determine motor behavior
        await c1.set_position(
            position= 0.2*math.sin(current_time*0.5)-0.1 if target_value else c1_position,
            accel_limit=1.5,
            velocity_limit=0.25,
            feedforward_torque=0,
        )
        await c2.set_position(
            position= 0.1*(math.sin(current_time)+math.sin(current_time*2))+0.3 if target_value else c2_position,
            accel_limit=1.5,
            velocity_limit=0.75,
            feedforward_torque=current_feedforward_torque,
            kp_scale=1.7,
            kd_scale=0.8,
        )

        # Small delay to prevent excessive CPU usage
        await asyncio.sleep(0.05)

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Program interrupted.")
