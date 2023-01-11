#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "commons/protocol.h"
#include "logging.h"

int main(int argc, char **argv){
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
        //Create request message serialized buffer and send through pipe
        Request pub_request;
        pub_request.code = 1;
        pub_request.client_named_pipe_path[256];
        pub_request.box_name[32];
        strcpy(pub_request.client_named_pipe_path, argv[2]);
        strcpy(pub_request.box_name, argv[3]);
        char request_buffer[sizeof(Request)];
        sprintf(request_buffer, "%u%s%s", pub_request.code , pub_request.client_named_pipe_path, pub_request.box_name);
        // Write the serialized message to the FIFO
        int bytes_written = write(register_fifo_write, request_buffer, sizeof(request_buffer));
        if (bytes_written < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        //Como saber se foi aceite ou nao?
        //ler linhas do input e mandar pelo pipe
        Message message;
        char line[sizeof(message.message)];
        char message_buffer[sizeof(Message)];
        while (fgets(line, sizeof(line), stdin) != NULL){//assim esta a ler linha a linha?
            message.code = 9;
            strcpy(message.message, line);
            sprintf(message_buffer, "%u%s", message.code, message.message);
            write(worker_fifo_write, message_buffer, sizeof(message_buffer));
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
