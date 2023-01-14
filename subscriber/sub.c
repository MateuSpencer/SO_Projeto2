#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "../protocol/protocol.h"
#include "logging.h"

int main(int argc, char **argv) {
    int msgCtr = 0;

    if(argc == 4){
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
        Request request;
        request.code = 2;
        strcpy(request.client_named_pipe_path, argv[2]);
        strcpy(request.box_name, argv[3]);
        send_request( request, register_fifo_write);
        //Como saber se foi aceite ou nao?
        //TODO: catch SIGPIPE
        // Open pipe for reading (waits for someone to open it for writing)
        int worker_fifo_read = open(argv[2], O_RDONLY);
        if (worker_fifo_read == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        /*//Tirar Menagem do Pipe e por no stdout
        Message message;
        char message_buffer[sizeof(Message)];
        offset = 0;
        ssize_t bytes_read = read_fifo(worker_fifo_read, message_buffer, sizeof(message_buffer));
        memcpy(&code, message_buffer, sizeof(code));
        offset += sizeof(code);
        remove_strings_from_buffer(message_buffer + offset, message.message , sizeof(message.message));
        while(bytes_read > 0){//will exit once the pipe writer exits
            if( code != 10 ){
                printf("Invalid type of message received from box - %d\n",code);
            }else{
                //fprintf(stdout, "%s\n", message.message);
                msgCtr++;
                offset = 0;
                bytes_read = read_fifo(worker_fifo_read, message_buffer, sizeof(message_buffer));
                memcpy(&code, message_buffer, sizeof(code));
                offset += sizeof(code);
                remove_strings_from_buffer(message_buffer + offset, message.message , sizeof(message.message));
            }
        }
        */
        //deve processar o SIGINT
            //fechar sessão
        sleep(10);
        fprintf(stdout, "%d\n", msgCtr);
        close(worker_fifo_read);
        close(register_fifo_write);
        return 0;
    }
    
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
