HANDLE pipe_open(const char *PipeName) {
    char full_pipe_name[256];
    sprintf(full_pipe_name, "\\\\.\\pipe\\%s", PipeName);
    HANDLE handle = CreateNamedPipe(
        full_pipe_name,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1, 1024, 1024, 0, NULL
    ); 
    assert(handle != INVALID_HANDLE_VALUE);

    printf("Waiting for Unity to connect to pipe...");

    ConnectNamedPipe(handle, NULL);
    return(handle);
}

#define pipe_write_byte serial_write_byte

#define pipe_read_byte serial_read_byte

int pipe_num_bytes_ready_to_read(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    DWORD avail;
    PeekNamedPipe(handle, NULL, 0, NULL, &avail, NULL);
    return(avail);
}