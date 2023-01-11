#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "../commons/protocol.h"
#include "logging.h"

int main(int argc, char **argv) {
    int msgCtr = 0;

    if(argc == 4){
        int register_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        // Create request message serialized buffer
        Request sub_request;
        sub_request.code = 2;
        strcpy(sub_request.client_named_pipe_path, argv[2]);
        strcpy(sub_request.box_name, argv[3]);
        char request_buffer[sizeof(Request)];
        sprintf(request_buffer, "%u%s%s", sub_request.code , sub_request.client_named_pipe_path, sub_request.box_name);
        // Write the serialized message to the FIFO
        ssize_t bytes_written = write(register_fifo_write, request_buffer, sizeof(request_buffer));
        if (bytes_written < 0) {
            fprintf(stderr, "[ERR]: write failed\n");
            exit(EXIT_FAILURE);
        }
        //Como saber se foi aceite ou nao?
        //open worker_pipe so that the worker thread can open it for writing
        int worker_fifo_read = open(argv[1], O_RDONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //Tirar Menagem do Pipe e por no stdout
        Message message;
        char message_buffer[sizeof(Message)];
        long unsigned int offset = 0;
        ssize_t bytes_read = read_fifo(worker_fifo_read, message_buffer, sizeof(message_buffer));
        memcpy(&message.code, message_buffer + offset, sizeof(message.code));
        offset += sizeof(message.code);
        memcpy(message.message, message_buffer + offset, sizeof(message.message));
        //verificar opcode?
        while(bytes_read > 0){//will exit once the pipe writer exits
            fprintf(stdout, "%s\n", message.message);
            msgCtr++;
            offset = 0;
            bytes_read = read_fifo(worker_fifo_read, message_buffer, sizeof(message_buffer));
            memcpy(&message.code, message_buffer + offset, sizeof(message.code));
            offset += sizeof(message.code);
            memcpy(message.message, message_buffer + offset, sizeof(message.message));
        }
        
        //deve processar o SIGINT
            //fechar sessão
        fprintf(stdout, "%d\n", msgCtr);
        close(worker_fifo_read);
        close(register_fifo_write);
        return 0;
    }
    
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
