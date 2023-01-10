#ifndef __protocol__
#define __protocol__

#include <stdint.h>

typedef struct {
    uint8_t code;
    char client_named_pipe_path[256];
    char box_name[32];
} RequestMessage;

//para respostas

//para listagens de caixas

//maybe criar uma função para serializar respostas e as enviar, e inverso

#endif __protocol__