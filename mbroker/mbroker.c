#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>

#include "../fs/operations.h"
#include "logging.h"
#include "../commons/protocol.h"
#include "producer-consumer.h"

volatile sig_atomic_t stop_workers = 0;
//volatile sig_atomic_t to make it thread safe

void *worker_thread_func(void *arg);

int main(int argc, char **argv){

    if(argc == 3){
        pc_queue_t queue;
        int num_threads = atoi(argv[2]);
        if (pcq_create(&queue, (size_t)(2*num_threads)) != 0){
        fprintf(stderr, "Error creating producer-consumer queue\n");
        return 1;
        }
        if(access(argv[1], F_OK) == 0) {
            if(unlink(argv[1]) == -1) {
                fprintf(stderr, "[ERR]: unlink(%s) failed\n", argv[1]);
            }
        }
        //criar register fifo: S_IWUSR(read permision for owner), S_IWOTH(write permisiions for others) - maybe other permissions needed
        if (mkfifo(argv[1], 0640) != 0) {
            fprintf(stderr, "[ERR]: mkfifo failed\n");
            exit(EXIT_FAILURE);
        }
        // Dynamically allocate memory for the thread array
        
        pthread_t *worker_threads = malloc((unsigned long int)num_threads * sizeof(pthread_t));
        if (worker_threads == NULL) {
            fprintf(stderr, "Error allocating memory for threads\n");
            return 1;
        }
        // Create all worrker_threads
        for (int i = 0; i < num_threads; i++) {
            if (pthread_create(&worker_threads[i], NULL, worker_thread_func, &queue) != 0) {
                fprintf(stderr, "Error creating worker thread\n");
                return 1;
            }
        }
        // Open pipe for reading (waits for someone to open it for writing)
        int register_fifo_read = open(argv[1], O_RDONLY);
        if (register_fifo_read == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        char buffer[sizeof(Request)];
        ssize_t bytes_read;
        int end = 0;
        while(end == 0){
            bytes_read = read(register_fifo_read, buffer, sizeof(buffer));
            printf("Received request - %lu\n", bytes_read);
            if (pcq_enqueue(&queue, buffer) != 0) {
                fprintf(stderr, "Error enqueuing request\n");
                return 1;
            }
        }
        if (bytes_read < 0){//error
            fprintf(stderr, "[ERR]: read failed1\n");
            exit(EXIT_FAILURE);
        }
        close(register_fifo_read);

        //Make the worker threads terminate
        stop_workers = 1;
        // Wait for the worker threads to complete
        for (int i = 0; i < num_threads; i++) {
            if (pthread_join(worker_threads[i], NULL) != 0) {
                fprintf(stderr, "Error waiting for worker thread\n");
                return 1;
            }
        }
        // Destroy the producer-consumer queue
        if (pcq_destroy(&queue) != 0) {
            fprintf(stderr, "Error destroying producer-consumer queue\n");
            return 1;
        }
        free(worker_threads);
        return 0;
    }

    fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions>\n");
    return -1;
}


void *worker_thread_func(void *arg) {
    pc_queue_t *queue = (pc_queue_t*)arg;
    long unsigned int offset = 0;
    uint8_t code = 0;
    Request request;
    int worker_fifo_write;
    int worker_fifo_read;
    while (!stop_workers) {
        // Dequeue a request
        char *request_buffer = (char*)pcq_dequeue(queue);
        // Process the request
        //ler so o primerio byte
        memcpy(&code, request_buffer, sizeof(code));
        offset += sizeof(code);
        switch (code){
            case 1: //Received request for publisher registration                
                printf("-----%d\n", code);
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                printf("-----%s\n", request.client_named_pipe_path);
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                printf("-----%s\n", request.box_name);
                // Open pipe for reading (waits for someone to open it for writing)
                worker_fifo_read = open(request.client_named_pipe_path, O_RDONLY);
                if (worker_fifo_read == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                
                //recebe estas e guarda na caixa
                //[ code = 9 (uint8_t) ] | [ message (char[1024]) ]
                
                break;
            case 2: //Received request for subscriber registration
                printf("-----%d\n", code);
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                printf("-----%s\n", request.client_named_pipe_path);
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                printf("-----%s\n", request.box_name);
                worker_fifo_write = open(request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                //le da caixa e envia estas
                //[ code = 10 (uint8_t) ] | [ message (char[1024]) ]
                

                break;
            case 3: //Received request for box creation
                printf("-----%d\n", code);
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                printf("-----%s\n", request.client_named_pipe_path);
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                printf("-----%s\n", request.box_name);
                worker_fifo_write = open(request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                //tfs_open(nome_do_worker_pipe., TFS_O_CREAT);
                //Resposta: [ code = 4 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ] 
            
                break;
            case 5: //Received request for box removal
                printf("-----%d\n", code);
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                printf("-----%s\n", request.client_named_pipe_path);
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                printf("-----%s\n", request.box_name);
                worker_fifo_write = open(request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                //Resposta: [ code = 6 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]

            
                break;
            case 7: //Received request for box listing

                //Resposta: varias mensagens assim
                    //[ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]

            
                break;
            default:
                printf("Received unknown message with code %u\n", code);
                break;
        }
    }
    return NULL;
}
