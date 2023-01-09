#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "commons/protocol.h"
#include "logging.h"

int main(int argc, char **argv) {

    if(argc == 4){
        //Open register fifo for writing request
        int register_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        // Create request message
        RequestMessage requestMessage;
        requestMessage.code = 1;
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

        //ficar a receber do stdin e  a mandar mensagens pelo pipe com o formato certo para ser processado
            //nao espera ativa ofc
            //[ code = 9 (uint8_t) ] | [ message (char[1024]) ]
            //Acaba quando apanhar o EOF tipo Ctrl+D e acabar a sessao como deve ser

        close(register_fifo_write);
        return 0;
    }

    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
