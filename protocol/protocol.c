#include "protocol.h"

#include <string.h>

ssize_t read_fifo(int fifo, char *buffer, size_t n_bytes){
    ssize_t bytes_read = read(fifo, buffer, n_bytes);
    if (bytes_read == -1) {
        return -1;
    }
    return bytes_read;
}

void store_string_in_buffer(char* buffer, char* str1, size_t space) {
    size_t str1_len = strlen(str1);
    size_t len = str1_len > space ? space : str1_len;//truncar strings demasiado grandes
    memcpy(buffer, str1, len);
    if (str1_len < space) {
        memset(buffer + str1_len, '\0', space - str1_len);
    }
}

void remove_strings_from_buffer(char* buffer, char* str1, size_t space) {
    size_t i;
    for (i = 0; i < space; i++) {
        if (buffer[i] == '\0') {
            break;
        }
        str1[i] = buffer[i];
    }
    str1[i] = '\0';
}

void send_request(Request request, int fifo){
    long unsigned int offset = 0;
    char request_buffer [sizeof(Request)];
    memcpy(request_buffer, &request.code, sizeof(request.code));
    offset += sizeof(request.code);
    store_string_in_buffer(request_buffer + offset, request.client_named_pipe_path, sizeof(request.client_named_pipe_path));
    offset += sizeof(request.client_named_pipe_path);
    store_string_in_buffer(request_buffer + offset, request.box_name, sizeof(request.box_name));
    // Write the serialized message to the FIFO
    ssize_t bytes_written = write(fifo, request_buffer, sizeof(request_buffer));
    if (bytes_written < 0) {
        fprintf(stderr, "[ERR]: write failed\n");
        exit(EXIT_FAILURE);
    }
}


void send_box_response(Box_Response response, int fifo){
    long unsigned int offset = 0;
    char response_buffer [sizeof(Box_Response)];
    memcpy(response_buffer, &response.code, sizeof(response.code));
    offset += sizeof(response.code);
    memcpy(response_buffer + offset, &response.return_code, sizeof(response.return_code));
    offset += sizeof(response.return_code);
    store_string_in_buffer(response_buffer + offset, response.error_message, sizeof(response.error_message));
    // Write the serialized message to the FIFO
    ssize_t bytes_written = write(fifo, response_buffer, sizeof(response_buffer));
    if (bytes_written < 0) {
        fprintf(stderr, "[ERR]: write failed\n");
        exit(EXIT_FAILURE);
    }
}