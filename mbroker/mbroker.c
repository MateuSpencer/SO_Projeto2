#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "logging.h"
#include "commons/protocol.h"
#include "producer-consumer.h"

volatile sig_atomic_t stop_workers = 0;
//volatile sig_atomic_t to make it thread safe

void *worker_thread_func(void *arg);

int main(int argc, char **argv){

    if(argc == 3){
        pc_queue_t queue;
        if (pcq_create(&queue, argv[2]) != 0){//TODO: qual é o tamanho da queue?
        fprintf(stderr, "Error creating producer-consumer queue\n");
        return 1;
        }
        //Remover pipe se ja existir
        if (unlink(argv[1]) != 0 && errno != ENOENT) {
            fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1], strerror(errno));
            exit(EXIT_FAILURE);
        }
        //criar register fifo: S_IWUSR(read permision for owner), S_IWOTH(write permisiions for others) - maybe other permissions needed
        if (mkfifo(argv[1], S_IRUSR | S_IWOTH) != 0) {
            fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        // Dynamically allocate memory for the thread array
        int num_threads = atoi(argv[2]);
        pthread_t *worker_threads = malloc(num_threads * sizeof(pthread_t));
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
        // Open pipe for writing in a different process
        pid_t pid = fork();
        if (pid == 0) {
            int register_fifo_ghost_writer = open(argv[1], O_WRONLY);
            //se o servidor acabar isto devia ser terminado tho. - é tirar este, e assim quando os outros sairem todos o loop abaixo acaba
            //maybe uma condição de variavel, a depender de apanhar um SIGINT ou assim
        }
        // Open pipe for reading (waits for someone to open it for writing)
        int register_fifo_read = open(argv[1], O_RDONLY);
        if (register_fifo_read == -1){
            fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        char buffer[sizeof(RequestMessage)];
        ssize_t bytes_read = read(register_fifo_read, buffer, sizeof(buffer));
        while(bytes_read > 0){
            if (pcq_enqueue(&queue, buffer) != 0) {
                fprintf(stderr, "Error enqueuing request\n");
                return 1;
            }
            bytes_read = read(register_fifo_read, buffer, sizeof(buffer));
        }
        if (bytes_read < 0){//error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
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
    while (!stop_workers) {
        // Dequeue a request
        char *request = (char*)pcq_dequeue(queue);
        // Process the request
        RequestMessage message;
        sscanf(request, "%u%s%s", &message.code, message.client_named_pipe_path, message.box_name);
        switch (message.code){
            case 1: //Received request for publisher registration
                
                break;
            case 2: //Received request for subscriber registration


                break;
            case 3: //Received request for box creation

            
                break;
            default:
                printf("Received unknown message with code %u\n", message.code);
                break;
        }

            //Resposta ao pedido de criação de caixa:[ code = 4 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
                //return ée 0 ou -1
                    //se for erro envia mensagem de erro, senao inicializa com \0
            //Resposta ao pedido de remoção de caixa:[ code = 6 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]

            //A resposta à listagem de caixas vem em várias mensagens, do seguinte tipo:
                //[ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
                //last é um se for a ultima da listagem, senao 0
                //box size é o tamanho em bytes da caixa
            
            //O servidor envia mensagens para o subscritor do tipo:[ code = 10 (uint8_t) ] | [ message (char[1024]) ]


        free(request);
    }
    return NULL;
}
