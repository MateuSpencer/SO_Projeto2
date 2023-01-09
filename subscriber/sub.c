#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "logging.h"

int main(int argc, char **argv) {
    int msgCtr = 0;

    if(argc == 4){

        int register_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //enviar pelo argv[1] (resgiter pipe) um pedido para se subscrever à caixa argv[3], e dar o nome do fifo entre a thred e este processo argv[2]
            //[ code = 2 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
        //le as mensagens que ja estao na caixa

        //fica a espera  de novas mensagens serem escritas
            //nao espera ativa
            //fprintf(stdout, "%s\n", message);

        //deve processar o SIGINT
            //fechar sessão
            //escrever numero de mensagens recebidas


    }
    
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
