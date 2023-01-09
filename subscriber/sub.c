#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "commons/protocol.h"
#include "logging.h"

int main(int argc, char **argv) {
    int msgCtr = 0;

    if(argc == 4){
        int register_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        // Create request message
        RequestMessage requestMessage;
        requestMessage.code = 2;
        strcpy(requestMessage.client_named_pipe_path, argv[2]);
        strcpy(requestMessage.box_name, argv[3]);
        // Serialize the message into a buffer
        char buffer[sizeof(RequestMessage)];
        sprintf(buffer, "%u%s%s", requestMessage.code, requestMessage.client_named_pipe_path, requestMessage.box_name);
        // Write the serialized message to the FIFO
        int bytes_written = write(register_fifo_write, buffer, sizeof(buffer));
        if (bytes_written < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        //Como saber se foi aceite ou nao?
        //acho que ja devia ter criado o pipe para falar com a thread, ou so agora?

        //le as mensagens que ja estao na caixa

        //fica a espera  de novas mensagens serem escritas no fifo
            //nao espera ativa
            //fprintf(stdout, "%s\n", message);

        //deve processar o SIGINT
            //fechar sessão
            //escrever numero de mensagens recebidas

    }
    
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
