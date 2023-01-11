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
    }else if(argc == 4){
        Message requestMessage;
        requestMessage.code = 0;
        if(strcmp(argv[2],"create")){
            requestMessage.code = 3;
        }else if(strcmp(argv[2],"remove")){ 
            requestMessage.code = 5;
        }
        if(requestMessage.code != 0){
            int register_fifo_write = open(argv[1], O_WRONLY);
            if (register_fifo_write == -1){
                fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            // Create request message
            strcpy(requestMessage.registration_request.client_named_pipe_path, argv[2]);
            strcpy(requestMessage.registration_request.box_name, argv[3]);
            // Serialize the message into a buffer
            char buffer[sizeof(Message)];
            sprintf(buffer, "%u%s%s", requestMessage.code, requestMessage.registration_request.client_named_pipe_path, requestMessage.registration_request.box_name);
            // Write the serialized message to the FIFO
            int bytes_written = write(register_fifo_write, buffer, sizeof(buffer));
            if (bytes_written < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            //abrir em leitura o fifo da worker thread para receber reposta
            int worker_fifo_read = open(argv[2], O_RDONLY);
            if (register_fifo_write == -1){
                fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            //ler resposta
            Message boxResponse;
            char worker_buffer[sizeof(Message)];
            ssize_t bytes_read = read(worker_fifo_read, worker_buffer, sizeof(worker_buffer));
            if (bytes_read < 0){//error
                fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            sscanf(worker_buffer, "%u%d%s", &boxResponse.code, boxResponse.box_response.return_code,boxResponse.box_response.error_message);
            //verificar opcode?
            if(boxResponse.box_response.return_code == 0){
                fprintf(stdout, "OK\n");
            }else if(boxResponse.box_response.return_code == -1){
                fprintf(stdout, "ERROR %s\n", boxResponse.box_response.error_message);
            }else{
                //resposta desconhecida
            }
            close(worker_fifo_read);
            close(register_fifo_write);
            return 0;
        }
    }
    
    print_usage();
    return -1;
}