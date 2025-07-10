HANDLE serial_open(const char *COMXX, int baud_rate) {
    char full_port_name[20];
    snprintf(full_port_name, sizeof(full_port_name), "\\\\.\\%s", COMXX);

    HANDLE result = CreateFileA(full_port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    assert(result != INVALID_HANDLE_VALUE);

    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb); // TODO: wtf
    assert(GetCommState(result, &dcb));
    dcb.BaudRate = baud_rate;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    assert(SetCommState(result, &dcb));

    // TODO: look into this
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 10;
    timeouts.ReadTotalTimeoutConstant = 10;
    timeouts.ReadTotalTimeoutMultiplier = 1;
    timeouts.WriteTotalTimeoutConstant = 10;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    assert(SetCommTimeouts(result, &timeouts));

    return(result);
}

void serial_close(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    CloseHandle(handle);
}

void serial_write_byte(HANDLE handle, u8 byte) {
    assert(handle != INVALID_HANDLE_VALUE);
    u32 bytes_written;
    WriteFile(handle, &byte, 1, &bytes_written, NULL);
    assert(bytes_written == 1);
}

u8 serial_read_byte(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    u8 byte;
    u32 bytes_read;
    ReadFile(handle, &byte, 1, &bytes_read, NULL);
    assert(bytes_read == 1);
    return(byte);
}

int serial_num_bytes_ready_to_read(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    u32 errors;
    COMSTAT status;
    assert(ClearCommError(handle, &errors, &status));
    return((int) status.cbInQue);
}
