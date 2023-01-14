#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>


#include "../protocol/protocol.h"
#include "logging.h"

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    long unsigned int offset = 0;
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
        ListingRequest listing_request;
        char listing_buffer [sizeof(ListingRequest)];
        memcpy(listing_buffer, &listing_request.code, sizeof(listing_request.code));
        offset += sizeof(listing_request.code);
        store_string_in_buffer(listing_buffer + offset, listing_request.client_named_pipe_path, sizeof(listing_request.client_named_pipe_path));
        ssize_t bytes_written = write(argv[1], listing_buffer, sizeof(listing_buffer));
        if (bytes_written < 0) {
            fprintf(stderr, "[ERR]: write failed\n");
            exit(EXIT_FAILURE);
        }
        
        //receber a resposta, ordenar, imprimir

        return 0;
    }else if(argc == 5){//Ou criar ou remover caixa
        
        uint8_t code = 0;
        if(strcmp(argv[3],"create") == 0){
            code = 3;
        }else if(strcmp(argv[3],"remove") == 0){ 
            code = 5;
        }
        if(code != 0){
            
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
            request.code = code;
            strcpy(request.client_named_pipe_path, argv[2]);
            strcpy(request.box_name, argv[4]);
            send_request( request, register_fifo_write);
            
            // Open pipe for reading (waits for someone to open it for writing)
            int worker_fifo_read = open(argv[2], O_RDONLY);
            if (worker_fifo_read == -1){
                fprintf(stderr, "[ERR]: open failed\n");
                exit(EXIT_FAILURE);
            }
            //ler resposta
            Box_Response box_response;
            char response_buffer[sizeof(Box_Response)];
            //read the serialized message
            ssize_t bytes_read = read_fifo(worker_fifo_read, response_buffer, sizeof(Box_Response));
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