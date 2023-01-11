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
        //open worker_pipe so that the worker thread can open it for reading
        int worker_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        // Create request message
        Message requestMessage;
        requestMessage.code = 1;
        strcpy(requestMessage.registration_request.client_named_pipe_path, argv[2]);
        strcpy(requestMessage.registration_request.box_name, argv[3]);
        // Serialize the message into a buffer
        char request_buffer[sizeof(Message)];
        sprintf(request_buffer, "%u%s%s", requestMessage.code, requestMessage.registration_request.client_named_pipe_path, requestMessage.registration_request.box_name);
        // Write the serialized message to the FIFO
        int bytes_written = write(register_fifo_write, request_buffer, sizeof(request_buffer));
        if (bytes_written < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        //Como saber se foi aceite ou nao?
        
        Message message;
        char line[sizeof(message.message)];
        char worker_buffer[sizeof(Message)];
        while (fgets(line, sizeof(line), stdin) != NULL){//fazer nao espera ativa, assim esta a ler linha a linha?
            message.code = 9;
            strcpy(message.message, line);
            sprintf(request_buffer, "%u%s%s", message.code, message.message.message);
            write(worker_fifo_write, worker_buffer, sizeof(worker_buffer));
            if (bytes_written < 0) {
                fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
        //Acaba quando apanhar o EOF tipo Ctrl+D e acabar a sessao como deve ser
        close(worker_fifo_write);
        close(register_fifo_write);
        return 0;
    }

    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
