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

void store_string_in_buffer(char* buffer, char* str1, size_t space) {
    // Copy first string to first 5 bytes of buffer
    size_t str1_len = strlen(str1);
    size_t len = str1_len > space ? space : str1_len;//truncar strings demasiado grandes
    memcpy(buffer, str1, len);
    // Fill remaining bytes with '\0' if the string is less than 5 bytes
    if (str1_len < 5) {
        memset(buffer + str1_len, '\0', 5 - str1_len);
    }
}

void remove_strings_from_buffer(char* buffer, char* str1, size_t space) {
    // Find the first null character in the first 5 bytes of buffer
    size_t i;
    for (i = 0; i < space; i++) {
        if (buffer[i] == '\0') {
            break;
        }
        str1[i] = buffer[i];
    }
    str1[i] = '\0';
}

#endif