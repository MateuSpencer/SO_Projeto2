#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>

#include "../protocol/protocol.h"
#include "logging.h"

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

void sigpipe_handler(int signum) {
    printf("SIGPIPE: %d\n", signum);
}

int main(int argc, char **argv) {
    long unsigned int offset = 0;
    ssize_t bytes_read;
    BoxData *box_data;
    if (signal(SIGPIPE, sigpipe_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
        }
    if(argc == 4 && strcmp(argv[3],"list") == 0){
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
        ListingRequest listing_request;
        char listing_buffer [sizeof(ListingRequest)];
        listing_request.code = 7;
        memcpy(listing_buffer, &listing_request.code, sizeof(listing_request.code));
        offset += sizeof(listing_request.code);
        strcpy(listing_request.client_named_pipe_path, pipe_name);
        store_string_in_buffer(listing_buffer + offset, listing_request.client_named_pipe_path, sizeof(listing_request.client_named_pipe_path));
        ssize_t bytes_written = write(register_fifo_write, listing_buffer, sizeof(listing_buffer));
        if (bytes_written < 0) {
            fprintf(stderr, "[ERR]: write failed\n");
            exit(EXIT_FAILURE);
        }
        close(register_fifo_write);
        // Open pipe for reading (waits for someone to open it for writing)
        int worker_fifo_read = open(pipe_name, O_RDONLY);
        if (worker_fifo_read == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        //lista para suportar toda a informação das caixas
        BoxList box_list_sort;
        box_list_sort.head = NULL;
        box_list_sort.tail = NULL;
        pthread_mutex_init(&box_list_sort.box_list_lock, NULL);
        //receber as respostas e por na lista para ordenar
        ListingResponse listing_response;
        char listing_response_buffer[sizeof(ListingResponse)];
        bytes_read = read_fifo(worker_fifo_read, listing_response_buffer, sizeof(ListingResponse));
        if(bytes_read != 0){
            while(bytes_read > 0){
                offset = 0;
                memcpy(&listing_response.code, listing_response_buffer, sizeof(listing_response.code));
                offset += sizeof(listing_response.code);
                //TODO verificar se codigo esta correto
                memcpy(&listing_response.last, listing_response_buffer + offset, sizeof(listing_response.last));
                offset += sizeof(listing_response.last);
                remove_strings_from_buffer(listing_response_buffer + offset, listing_response.box_name , sizeof(listing_response.box_name));
                offset += sizeof(listing_response.box_name);
                memcpy(&listing_response.box_size, listing_response_buffer + offset, sizeof(listing_response.box_size));
                offset += sizeof(listing_response.box_size);
                memcpy(&listing_response.n_publishers , listing_response_buffer + offset, sizeof(listing_response.n_publishers));
                offset += sizeof(listing_response.n_publishers );
                memcpy(&listing_response.n_subscribers  , listing_response_buffer + offset, sizeof(listing_response.n_subscribers));
                //adicionar informação na lista de boxes para ser ordenada
                insert_at_beginning(&box_list_sort, listing_response.box_name,listing_response.box_size, listing_response.n_publishers, listing_response.n_subscribers );

                bytes_read = read_fifo(worker_fifo_read, listing_response_buffer, sizeof(ListingResponse));
            }
            if (bytes_read < 0){//error
                fprintf(stderr, "[ERR]: read failed\n");
                exit(EXIT_FAILURE);
            }
            //ORDENAR

            //TODO

            //imprimir todos
            box_data = box_list_sort.head;
            while(box_data != NULL){
                fprintf(stdout, "%s %zu %zu %zu\n", box_data->box_name, box_data->box_size, box_data->n_publishers, box_data->n_subscribers);
                box_data = box_data->next;
            } 

            close(worker_fifo_read); 
            return 0;
        }
        fprintf(stdout, "NO BOXES FOUND\n");
        close(worker_fifo_read);
        return 0;
    }else if(argc == 5){//Ou criar ou remover caixa
        
        uint8_t code = 0;
        if(strcmp(argv[3],"create") == 0){
            code = 3;
        }else if(strcmp(argv[3],"remove") == 0){ 
            code = 5;
        }
        if(code != 0){
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
            request.code = code;
            strcpy(request.client_named_pipe_path, pipe_name);
            strcpy(request.box_name, argv[4]);
            send_request( request, register_fifo_write);
            
            // Open pipe for reading (waits for someone to open it for writing)
            int worker_fifo_read = open(pipe_name, O_RDONLY);
            if (worker_fifo_read == -1){
                fprintf(stderr, "[ERR]: open failed\n");
                exit(EXIT_FAILURE);
            }
            //ler resposta
            Box_Response box_response;
            char response_buffer[sizeof(Box_Response)];
            //read the serialized message
            bytes_read = read_fifo(worker_fifo_read, response_buffer, sizeof(Box_Response));
            if(bytes_read <0 ){
                //TODO
            }
            //read the message code
            memcpy(&box_response.code, response_buffer, sizeof(box_response.code));
            offset += sizeof(code);
            //TODO verificar se o codigo é 4 ou 6
            memcpy(&box_response.return_code, response_buffer + offset, sizeof(box_response.return_code));
            if(box_response.return_code == 0){
                fprintf(stdout, "OK\n");
            }else if(box_response.return_code == -1){
                //read the return error message
                offset += sizeof(box_response.return_code);
                remove_strings_from_buffer(response_buffer + offset, box_response.error_message , sizeof(box_response.error_message));
                fprintf(stdout, "ERROR %s\n", box_response.error_message);
            }else{
                printf("UNKNOWN BOX RESPONSE\n");
            }
            close(worker_fifo_read);
            close(register_fifo_write);
            return 0;
        }
    }
    
    print_usage();
    return -1;
}