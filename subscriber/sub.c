#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

#include "../protocol/protocol.h"
#include "logging.h"

int end;//declared globally so that the signal treatment can exit the loop and consequently the program

void sigint_handler(int signum) {
    (void)signum;
    printf("\n");
    end = 1;
}

int main(int argc, char **argv) {
    int msgCtr = 0;

    if(argc == 4){
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
            fprintf(stderr, "[ERR]: mkfifo failed\n");
            exit(EXIT_FAILURE);
        }
        //Create request message serialized buffer and send through pipe
        Request request;
        request.code = 2;
        strcpy(request.client_named_pipe_path, pipe_name);
        strcpy(request.box_name, argv[3]);
        send_request( request, register_fifo_write);

        // Open pipe for reading (waits for someone to open it for writing)
        int worker_fifo_read = open(pipe_name, O_RDONLY);
        if (worker_fifo_read == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //Tirar Menagem do Pipe e por no stdout
        Message message;
        char message_buffer[sizeof(Message)];
        long unsigned int offset = 0;
        end = 0;
        ssize_t bytes_read = read_fifo(worker_fifo_read, message_buffer, sizeof(message_buffer));//ignorar o 0 enviado pelo teste ou entao causar SIGPIPE
        if(strcmp(message_buffer, "test") == 0){//read test correctly
            bytes_read = read_fifo(worker_fifo_read, message_buffer, sizeof(message_buffer));
            memcpy(&message.code, message_buffer, sizeof(message.code));
            offset += sizeof(message.code);
            remove_strings_from_buffer(message_buffer + offset, message.message , sizeof(message.message));
            while(end == 0){//will exit once the pipe writer exits
                if( message.code != 10 ){
                    printf("Invalid type of message received from box - %d\n",message.code);
                }else{
                    fprintf(stdout, "%s\n", message.message);
                    msgCtr++;
                }
                offset = 0;
                bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));
                if(bytes_read == 0) break;
                bytes_read++;//TODO unused
                memcpy(&message.code, message_buffer, sizeof(message.code));
                offset += sizeof(message.code);
                remove_strings_from_buffer(message_buffer + offset, message.message , sizeof(message.message));
            }
        }else{
            printf("Failed to connect to Box\n");
        }
        fprintf(stdout, "%d\n", msgCtr);
        close(worker_fifo_read);
        close(register_fifo_write);
        return 0;
    }
    
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
