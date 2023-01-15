#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "../protocol/protocol.h"
#include "../utils/logging.h"

int end;

void sigpipe_handler(int signum){
    (void) signum;
    printf("BOX NOT FOUND\n");
    end = 1;
}

void sigint_handler(int signum) {
    (void) signum;
    end = 1;
}

int main(int argc, char **argv){
    if(argc == 4){
        
        if (signal(SIGPIPE, sigpipe_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
        }
        if (signal(SIGINT, sigint_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
        }
        //Append "../" to the beggining of the names to store worker pipes in a common place
        char register_pipe_name [strlen(argv[1]) + 3];
        sprintf(register_pipe_name, "../%s", argv[1]);
        char pipe_name [strlen(argv[2]) + 3];
        sprintf(pipe_name, "../%s", argv[2]);
        //Check if pipe_name already exists
        if(access(pipe_name, F_OK) == 0) {
            fprintf(stderr, "[ERR]: %s pipe_name alreay in use\n", argv[2]);
            exit(EXIT_FAILURE);
        }
        //Open register fifo for writing request
        int register_fifo_write = open(register_pipe_name, O_WRONLY);
        if (register_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //Create worker fifo
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
        close(register_fifo_write);
        //open worker_pipe for writing
        int worker_fifo_write = open(pipe_name, O_WRONLY);
        if (worker_fifo_write == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //wait for mbroker to decide if it is accepted, 
        //if this is not done sometimes it can write while the other end hast been closed yet but will close in the future
        struct timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = 10000000; //0.01seconds, could possibly be smaller
        nanosleep(&sleep_time, NULL);
        end = 0;
        //testar se foi aceite
        char message_test[] = "0";
        ssize_t bytes_written = write(worker_fifo_write, message_test, sizeof(message_test));//se a outra ponta tiver sido fechada porque  a sessao nao foi aceita SIGPIPE é lançado
        //ler linhas do input e mandar pelo pipe
        Message message;
        char line[sizeof(message.message)];
        char message_buffer[sizeof(Message)];
        long unsigned int offset = 0;
        message.code = 9;
        memcpy(message_buffer, &message.code, sizeof(message.code));
        offset += sizeof(message.code);
        while (end == 0){
            //read a line from sthe input
            char* read = fgets(line, sizeof(line), stdin);
            if(read == NULL)break;//Exit because of SIGINT
            //remove  the \n character from the end of the string
            char *newline = strchr(line, '\n');
            if (newline) {
                *newline = '\0';
            }
            //serialize message and send through pipe
            store_string_in_buffer(message_buffer + offset, line, sizeof(message.message));
            bytes_written = write(worker_fifo_write, message_buffer, sizeof(message_buffer));
            if (bytes_written < 0) {
                fprintf(stderr, "[ERR]: write failed\n");
                exit(EXIT_FAILURE);
            }
        }
        close(worker_fifo_write);
        if(unlink(pipe_name) == -1) {
            fprintf(stderr, "[ERR]: unlink(%s) failed\n", pipe_name);
        }
        return 0;
    }

    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
