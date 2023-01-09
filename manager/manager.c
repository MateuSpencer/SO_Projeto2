#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "commons/protocol.h"
#include "logging.h"

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

//TODO: Simplificar bue codigo repetido
int main(int argc, char **argv) {

    if(argc == 3){
        int register_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        //Pedido de listagem de caixas:
            //[ code = 7 (uint8_t) ] | [ client_named_pipe_path (char[256]) ]

        return 0;
    }else if(argc == 4){ //TODO: ta dumb, basicamente o codigo e igual para o create e remove, so muda o code de 3 ou 5
        if(strcmp(argv[2],"create")){
            int register_fifo_write = open(argv[1], O_WRONLY);
            if (register_fifo_write == -1){
                fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            //criar fifo para receber resposta

            // Create request message
            RequestMessage requestMessage;
            requestMessage.code = 3;
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

            //ler resposta do pipe
            //imprimir resposta
            //fechar pipes, acabar
            return 0;
        }else if(strcmp(argv[2],"remove")){
            
            if(strcmp(argv[2],"create")){
            int register_fifo_write = open(argv[1], O_WRONLY);
            if (register_fifo_write == -1){
                fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            //criar fifo para receber resposta

            // Create request message
            RequestMessage requestMessage;
            requestMessage.code = 5;
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

            //ler resposta do pipe
            //imprimir resposta
            //fechar pipes, acabar
            return 0;
        }
    }
    
    print_usage();
    return -1;
}
