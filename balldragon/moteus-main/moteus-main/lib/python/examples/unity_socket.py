import socket
import struct
import time
import random

host, port = "127.0.0.1", 25001
data = [2.9, 1.3, 1.6]

# SOCK_STREAM means TCP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

count = 5

sock.connect((host, port))

while(count > 0):
    data = ran_floats = [random.uniform(0,3) for _ in range(3)]

    # Connect to the server and send the data
    sock.sendall(struct.pack('<3f', *data))
    response = struct.unpack('<3f', sock.recv(1024))
    print (response)

    time.sleep(2)
    count -= 1

sock.close()