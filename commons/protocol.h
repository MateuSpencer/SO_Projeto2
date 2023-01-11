#ifndef __protocol__
#define __protocol__

#include <stdint.h>

typedef struct {
    uint8_t code;
    union {
        struct {
            char client_named_pipe_path[256];
            char box_name[32];
        } registration_request;
        struct {
            int32_t return_code;
            char error_message[1024];
        } box_response;
        struct {
            char client_named_pipe_path[256];
        } listing_request;
        struct {
            uint8_t last;
            char box_name[32];
            uint64_t box_size;
            uint64_t n_publishers;
            uint64_t n_subscribers;
        } listing_response;
        struct {
            char message[1024];
        } message;
    };
} Message;

//maybe criar uma função para serializar respostas e as enviar, e inverso

#endif __protocol__