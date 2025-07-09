#include <windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    HANDLE handle;
    bool connected;
} SerialPort;

bool open_serial(SerialPort *serial_port, const char *port_name, int baud_rate) {
    char full_port_name[20];
    snprintf(full_port_name, sizeof(full_port_name), "\\\\.\\%s", port_name);

    serial_port->handle = CreateFileA(
        full_port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (serial_port->handle == INVALID_HANDLE_VALUE) {
        serial_port->connected = false;
        return false;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(serial_port->handle, &dcbSerialParams)) {
        CloseHandle(serial_port->handle);
        return false;
    }

    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity   = NOPARITY;

    if (!SetCommState(serial_port->handle, &dcbSerialParams)) {
        CloseHandle(serial_port->handle);
        return false;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 10;
    timeouts.ReadTotalTimeoutConstant = 10;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 10;
    timeouts.WriteTotalTimeoutMultiplier = 1;

    SetCommTimeouts(serial_port->handle, &timeouts);

    serial_port->connected = true;
    return true;
}

void close_serial(SerialPort *serial_port) {
    if (serial_port->connected) {
        CloseHandle(serial_port->handle);
        serial_port->connected = false;
    }
}

bool write_byte(SerialPort *serial_port, uint8_t byte) {
    DWORD bytes_written;
    return WriteFile(serial_port->handle, &byte, 1, &bytes_written, NULL) && bytes_written == 1;
}

bool read_byte(SerialPort *serial_port, uint8_t *byte) {
    DWORD bytes_read;
    return ReadFile(serial_port->handle, byte, 1, &bytes_read, NULL) && bytes_read == 1;
}

int serial_available(SerialPort *serial_port) {
    DWORD errors;
    COMSTAT status;

    if (!ClearCommError(serial_port->handle, &errors, &status)) return -1;
    return (int)status.cbInQue;
}

int main(void) {
    SerialPort serial_port;

    if (!open_serial(&serial_port, "COM11", 115200)) {
        printf("Failed to open serial port COM11\n");
        return 1;
    }

    printf("Serial port COM11 opened at 115200 baud.\n");

    uint8_t byte_to_send = 'A';

    if (write_byte(&serial_port, byte_to_send)) {
        printf("Sent byte: %c\n", byte_to_send);
    } else {
        printf("Failed to write byte to serial port.\n");
    }

    close_serial(&serial_port);
    printf("Serial port closed.\n");

    return 0;
}
