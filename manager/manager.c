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
        
        uint8_t code = 0;
        if(strcmp(argv[2],"create")){
            code = 3;
        }else if(strcmp(argv[2],"remove")){ 
            code = 5;
        }
        if(code != 0){
            int register_fifo_write = open(argv[1], O_WRONLY);
            if (register_fifo_write == -1){
                fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
            //Create request message serialized buffer and send through pipe
            Request box_request;
            box_request.code = code;
            box_request.client_named_pipe_path[256];
            box_request.box_name[32];
            strcpy(box_request.client_named_pipe_path, argv[2]);
            strcpy(box_request.box_name, argv[3]);
            char request_buffer[sizeof(Request)];
            sprintf(request_buffer, "%u%s%s", box_request.code , box_request.client_named_pipe_path, box_request.box_name);
            // Write the serialized message to the FIFO
            int bytes_written = write(register_fifo_write, request_buffer, sizeof(request_buffer));
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
            Box_Response box_response;
            char response_buffer[1024];
            ssize_t bytes_read = read_fifo(worker_fifo_read, response_buffer, 1);
            code = atoi(response_buffer);
            if(code == 0){
                fprintf(stdout, "OK\n");
            }else if(code == -1){
                bytes_read = read_fifo(worker_fifo_read, response_buffer, (sizeof(response_buffer)-1));
                fprintf(stdout, "ERROR %s\n", response_buffer);
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