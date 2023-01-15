#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

#include "../protocol/protocol.h"
#include "logging.h"

int end;

void sigpipe_handler(int signum){
    (void) signum;
    printf("BOX NOT FOUND\n");
}

void sigint_handler(int signum) {
    (void) signum;
    end = 1;
}
//TODO  a criar os worker pipes em vez de se ja existir os apagar, dar erro que ja existe e para dar outro nome
int main(int argc, char **argv){
    if(argc == 4){
        
        if (signal(SIGPIPE, sigpipe_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
        }
        if (signal(SIGINT, sigint_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
        }
        char register_pipe_name [strlen(argv[1]) + 3];
        sprintf(register_pipe_name, "../%s", argv[1]);
        char pipe_name [strlen(argv[2]) + 3];
        sprintf(pipe_name, "../%s", argv[2]);
        //Open register fifo for writing request
        int register_fifo_write = open(register_pipe_name, O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //Create worker fifo
        if(access(pipe_name, F_OK) == 0) {
            if(unlink(pipe_name) == -1) {
                fprintf(stderr, "[ERR]: unlink(%s) failed\n", pipe_name);
            }
        }
        if (mkfifo(pipe_name, 0640) != 0) {
            fprintf(stderr, "[ERR]: mkfifo failed--\n");
            exit(EXIT_FAILURE);
        }
        //Create request message serialized buffer and send through pipe
        Request request;
        request.code = 1;
        strcpy(request.client_named_pipe_path, pipe_name);
        strcpy(request.box_name, argv[3]);
        send_request( request, register_fifo_write);
        
        //open worker_pipe for writing
        int worker_fifo_write = open(pipe_name, O_WRONLY);
        if (worker_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //testar se foi aceite -  se nao a funçao de hndle devia acabar com o publisher como deve ser
        char message_test[] = "0";
        ssize_t bytes_written = write(worker_fifo_write, message_test, sizeof(message_test));

        //ler linhas do input e mandar pelo pipe
        Message message;
        char line[sizeof(message.message)];
        char message_buffer[sizeof(Message)];
        
        end = 0;
        char* read = fgets(line, sizeof(line), stdin);
        while (end == 0){
            if(read == NULL){
                fprintf(stderr, "[ERR]: reading failed\n");
                exit(EXIT_FAILURE);
            }
            long unsigned int offset = 0;
            message.code = 9;
            memcpy(message_buffer, &message.code, sizeof(message.code));
            offset += sizeof(message.code);
            char *newline = strchr(line, '\n');
            if (newline) {
                *newline = '\0';
            }
            store_string_in_buffer(message_buffer + offset, line, sizeof(message.message));
            bytes_written = write(worker_fifo_write, message_buffer, sizeof(message_buffer));
            if (bytes_written < 0) {
                fprintf(stderr, "[ERR]: write failed\n");
                exit(EXIT_FAILURE);
            }
            read = fgets(line, sizeof(line), stdin);
        }

        close(worker_fifo_write);
        close(register_fifo_write);
        return 0;
    }

    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
