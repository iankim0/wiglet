HANDLE serial_open(const char *COMXX, int baud_rate) {
    char full_port_name[20];
    snprintf(full_port_name, sizeof(full_port_name), "\\\\.\\%s", COMXX);

    HANDLE handle = CreateFileA(
        full_port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    assert(handle != INVALID_HANDLE_VALUE );

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb); // TODO: wtf
    assert(GetCommState(handle, &dcb));
    dcb.BaudRate = baud_rate;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    assert(SetCommState(handle, &dcb));

    // TODO: look into this
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 10;
    timeouts.ReadTotalTimeoutConstant = 10;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 10;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    assert(SetCommTimeouts(handle, &timeouts));
    return(handle);
}

void serial_close(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    CloseHandle(handle);
}

void serial_write_byte(HANDLE handle, u8 byte) {
    assert(handle != INVALID_HANDLE_VALUE);
    DWORD bytes_written;
    WriteFile(handle, &byte, 1, &bytes_written, NULL);
    assert(bytes_written == 1);
}

void serial_read_byte(HANDLE handle, u8 *byte) {
    assert(handle != INVALID_HANDLE_VALUE);
    DWORD bytes_read;
    ReadFile(handle, byte, 1, &bytes_read, NULL);
    assert(bytes_read == 1);
}

void serial_write_n_bytes(HANDLE handle, u32 n, void *src) {
    assert(handle != INVALID_HANDLE_VALUE);
    DWORD bytes_written;
    WriteFile(handle, src, n, &bytes_written, NULL);
    assert(bytes_written == n);
}

void serial_read_n_bytes(HANDLE handle, u32 n, void *dest) {
    assert(handle != INVALID_HANDLE_VALUE);
    DWORD bytes_read;
    ReadFile(handle, dest, n, &bytes_read, NULL);
    assert(bytes_read == n);
}

int serial_available(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    DWORD errors;
    COMSTAT status;
    assert(ClearCommError(handle, &errors, &status));
    return((int) status.cbInQue);
}
