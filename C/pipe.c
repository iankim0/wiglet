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

void pipe_write_byte(HANDLE handle, u8 byte) {
    assert(handle != INVALID_HANDLE_VALUE);
    u32 bytes_written;
    WriteFile(handle, &byte, 1, &bytes_written, NULL);
    assert(bytes_written == 1);
}

u8 pipe_read_byte(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    u8 byte;
    u32 bytes_read;
    ReadFile(handle, &byte, 1, &bytes_read, NULL);
    assert(bytes_read == 1);
    return(byte);
}

int pipe_num_bytes_ready_to_read(HANDLE handle) {
    assert(handle != INVALID_HANDLE_VALUE);
    u32 avail;
    PeekNamedPipe(handle, NULL, 0, NULL, &avail, NULL);
    return(avail);
}