HANDLE pipe_create(const char *PipeName) {
    char full_pipe_name[256];
    sprintf(full_pipe_name, "\\\\.\\pipe\\%s", PipeName);
    HANDLE handle = CreateNamedPipe(
        full_pipe_name,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
        1, 1024, 1024, 0, NULL
    ); 
    assert(handle != INVALID_HANDLE_VALUE);
    return(handle);
}

bool pipe_attempt_to_connect(HANDLE handle) {
    BOOL result = ConnectNamedPipe(handle, NULL);
    if (!result) {
        DWORD err = GetLastError();
        if (err == ERROR_PIPE_CONNECTED) {
            return true;
        }
    }
    return false;
}

bool pipe_is_connected(HANDLE handle) {
    DWORD bytes_available;
    if (!PeekNamedPipe(handle, NULL, 0, NULL, &bytes_available, NULL)) {
        return false;  // Pipe disconnected or error
    }
    return true;
}

#define pipe_write_byte serial_write_byte
#define pipe_read_byte serial_read_byte
#define pipe_write_n_bytes serial_write_n_bytes
#define pipe_read_n_bytes serial_read_n_bytes

int pipe_available(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    DWORD avail;
    PeekNamedPipe(handle, NULL, 0, NULL, &avail, NULL);
    return(avail);
}