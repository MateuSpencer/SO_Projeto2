#ifndef __protocol__
#define __protocol__

#include <stdint.h>

typedef struct {
    uint8_t code;
    char client_named_pipe_path[256];
    char box_name[32];
} RequestMessage;

//para espostas

//para listagens de caixas

#endif __protocol__