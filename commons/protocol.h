#ifndef __protocol__
#define __protocol__

#include <stdint.h>
#include <unistd.h>

typedef struct  {
    uint8_t code;
    char client_named_pipe_path[256];
    char box_name[32];
}Request;
typedef struct  {
    uint8_t code;
    char client_named_pipe_path[256];
}ListingRequest;
typedef struct  {
    uint8_t code;
    uint8_t last;
    char box_name[32];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
}ListingResponse;

typedef struct  {
    uint8_t code;
    char message[1024];
}Message;

typedef struct  {
    uint8_t code;
    int32_t return_code;
    char error_message[1024];
}Box_Response;

ssize_t read_fifo(int fifo, char *buffer, size_t n_bytes){
    ssize_t bytes_read = read(fifo, buffer, n_bytes);
    if (bytes_read == -1) {
        return -1;
    }
    return bytes_read;
}


#endif