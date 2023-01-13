#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

#include "../commons/protocol.h"
#include "logging.h"

void sigpipe_handler(int signum) {
    printf("SIGPIPE: %d\n", signum);
}

int main(int argc, char **argv){
    if(argc == 4){
        
        if (signal(SIGPIPE, sigpipe_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
        }
        //Open register fifo for writing request
        int register_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //Create worker fifo
        if(access(argv[2], F_OK) == 0) {
            if(unlink(argv[2]) == -1) {
                fprintf(stderr, "[ERR]: unlink(%s) failed\n", argv[2]);
            }
        }
        if (mkfifo(argv[2], 0640) != 0) {
            fprintf(stderr, "[ERR]: mkfifo failed--\n");
            exit(EXIT_FAILURE);
        }
        //Create request message serialized buffer and send through pipe
        long unsigned int offset = 0;
        char request_buffer [sizeof(Request)];
        Request request;
        uint8_t code = 1;
        memcpy(request_buffer, &code, sizeof(code));
        offset += sizeof(code);
        store_string_in_buffer(request_buffer + offset, argv[2], sizeof(request.client_named_pipe_path));
        offset += sizeof(request.client_named_pipe_path);
        store_string_in_buffer(request_buffer + offset, argv[3], sizeof(request.box_name));
        // Write the serialized message to the FIFO
        ssize_t bytes_written = write(register_fifo_write, request_buffer, sizeof(request_buffer));
        if (bytes_written < 0) {
            fprintf(stderr, "[ERR]: write failed\n");
            exit(EXIT_FAILURE);
        }
        //Como saber se foi aceite ou nao?
        //open worker_pipe for writing
        int worker_fifo_write = open(argv[2], O_WRONLY);
        if (worker_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //testar se foi aceite -  se nao a funçao de hndle devia acabar com o projeto como deve ser
        char message_test[] = "0";
        bytes_written = write(worker_fifo_write, message_test, sizeof(message_test));

        //ler linhas do input e mandar pelo pipe
        Message message;
        char line[sizeof(message.message)];
        char message_buffer[sizeof(Message)];
        while (fgets(line, sizeof(line), stdin) != NULL){//assim esta a ler linha a linha?
            offset = 0;
            code = 9;
            memcpy(message_buffer, &code, sizeof(code));
            offset += sizeof(code);
            store_string_in_buffer(request_buffer + offset, line, sizeof(message.message));
            bytes_written = write(worker_fifo_write, message_buffer, sizeof(message_buffer));
            if (bytes_written < 0) {
                fprintf(stderr, "[ERR]: write failed\n");
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
