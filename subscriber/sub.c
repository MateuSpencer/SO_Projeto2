#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "commons/protocol.h"
#include "logging.h"

int main(int argc, char **argv) {
    int msgCtr = 0;

    if(argc == 4){
        int register_fifo_write = open(argv[1], O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        // Create request message
        RequestMessage requestMessage;
        requestMessage.code = 2;
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
        //Como saber se foi aceite ou nao?
        //open worker_pipe so that the worker thread can open it for reading
        int worker_fifo_read = open(argv[1], O_RDONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        Message message;
        char worker_buffer[sizeof(Message)];
        ssize_t bytes_read = read(worker_fifo_read, worker_buffer, sizeof(worker_buffer));
        while(bytes_read > 0){//will exit once the pipe writer exits
            //fazer nao espera ativa
            sscanf(worker_buffer, "%u%s%s", &message.code, message.message);
            //verificar opcode?
            fprintf(stdout, "%s\n", message.message);
            bytes_read = read(worker_fifo_read, buffer, sizeof(buffer));
        }
        if (bytes_read < 0){//error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        //deve processar o SIGINT
            //fechar sessão
            //escrever numero de mensagens recebidas
        return 0;
    }
    
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
