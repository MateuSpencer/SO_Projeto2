#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "logging.h"

int main(int argc, char **argv) {

    if(argc == 4){
        
        int register_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        //enviar pelo argv[1] (resgiter pipe) um pedidi para se criar ums sessao de publicar na caixa argv[3], e dar o nome do fifo entre a thred e este processo argv[2]
            //[ code = 1 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
            //pode nao ser aceite
        //ficar a escrever so stdin e  a mandar pelo pip com o formato certo para ser processado
            //[ code = 9 (uint8_t) ] | [ message (char[1024]) ]

        //tem de apanhar o EOF tico Ctrl+D e acabar a sessao
    }

    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
