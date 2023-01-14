#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <assert.h>

#include "../fs/operations.h"
#include "logging.h"
#include "../protocol/protocol.h"
#include "producer-consumer.h"


void *worker_thread_func(void *arg);

//volatile sig_atomic_t to make it thread safe
volatile sig_atomic_t stop_workers = 0;

//Global variable for list of boxes
BoxList box_list;

int main(int argc, char **argv){

    if(argc == 3){
        box_list.head = NULL;
        box_list.tail = NULL;
        //Initialize TFS
        assert(tfs_init(NULL) != -1);
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
        //open one end with a ghost writer so it always has a writer
        int register_fifo_ghost_writer = open(argv[1], O_WRONLY);
        if (register_fifo_read == -1){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        register_fifo_ghost_writer++;//TODO: unused
        register_fifo_ghost_writer--;

        char buffer[sizeof(Request)];
        ssize_t bytes_read;
        int end = 0;
        while(end == 0){
            bytes_read = read(register_fifo_read, buffer, sizeof(buffer));
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
    ssize_t bytes_read;
    ssize_t bytes_written;
    uint8_t code = 0;
    Request request;
    Box_Response box_reponse;
    char new_box_name[40];
    Message message;
    char message_buffer[sizeof(Message)];
    ListingRequest listing_request;
    ListingResponse listing_response;
    char listing_buffer[sizeof(ListingResponse)];
    int worker_fifo_write;
    int worker_fifo_read;
    BoxData *box_data;

    while (!stop_workers) {
        pthread_t thread_id = pthread_self();//TODO
        // Dequeue a request
        char *request_buffer = (char*)pcq_dequeue(queue);
        offset = 0;
        //ler so o primerio byte
        printf("Thread ID: %ld\n", thread_id);
        memcpy(&code, request_buffer, sizeof(code));
        offset += sizeof(code);
        // Process the request
        switch (code){
            case 1: //Received request for publisher registration                
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                
                // Open pipe for reading (waits for someone to open it for writing)
                //como registar quantos publishers cada caixa tem
                worker_fifo_read = open(request.client_named_pipe_path, O_RDONLY);
                if (worker_fifo_read == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                //Adicionar / ao inicio do nome da Box e tentar abrira a box
                sprintf(new_box_name, "/%s", request.box_name);
                int publisher_file_handle = tfs_open(new_box_name, TFS_O_APPEND);
                box_data = find_box(&box_list, new_box_name);
                if(box_data != NULL && publisher_file_handle >= 0 && box_data->n_publishers == 0 ){//ir logo para o close e dar SIGPIPE
                    bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));// para o teste caso nao tenha closed, ignorar o 0 mandado
                    box_data->n_publishers = 1;
                    bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));
                    while(bytes_read > 0){
                        memcpy(&message.code, message_buffer, sizeof(message.code));
                        offset = 0;
                        offset += sizeof(message.code);
                        remove_strings_from_buffer(message_buffer + offset, message.message , sizeof(message.message));
                        tfs_write(publisher_file_handle, message.message, sizeof(message.message));// escrever um \0 a seguir? ou ja vem com isso? ou nem preciso pq vou sempre ler o mm tamanho
                        //aumentar tamanho da caixa
                        size_t message_len = strlen(message.message);
                        box_data->box_size += message_len;
                        //ler proxima mensagem
                        bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));
                    }
                    if (bytes_read < 0){//error
                        fprintf(stderr, "[ERR]: read failed1\n");
                        exit(EXIT_FAILURE);
                    }
                    box_data->n_publishers = 0;
                }
                close(worker_fifo_read);
                break;
            case 2: //Received request for subscriber registration
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                
                worker_fifo_write = open(request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                sprintf(new_box_name, "/%s", request.box_name);
                int subscriber_file_handle = tfs_open(new_box_name, 0);
                //ler as mensagens todas que ja la estao
                // entrar em espera dependendo de uma variavel de condição e ler quando eal recebe signal
                //como acabar? se a outra fechar a ponta de leitura esta recebe sigpipe
                //
                bytes_read = tfs_read(subscriber_file_handle, message_buffer, sizeof(message.message));

                if (bytes_read < 0){//error
                        fprintf(stderr, "[ERR]: read failed1\n");
                        exit(EXIT_FAILURE);
                    }
                //[ code = 10 (uint8_t) ] | [ message (char[1024]) ]
                

                break;
            case 3: //Received request for box creation
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                worker_fifo_write = open(request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                
                sprintf(new_box_name, "/%s", request.box_name);
                box_reponse.code = 4;
                int create = tfs_open(new_box_name, TFS_O_CREAT);
                
                if(create == -1){
                    box_reponse.return_code = -1;
                    strcpy(box_reponse.error_message, "ganda erro a criar");//TODO
                }else{
                    box_reponse.return_code = 0;
                    strcpy(box_reponse.error_message, "\0");
                    //adicionar box a lista
                    insert_at_beginning(&box_list, new_box_name, 0, 0, 0);
                }                
                send_box_response(box_reponse, worker_fifo_write);
                close(worker_fifo_write);
                break;
            case 5: //Received request for box removal
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                
                worker_fifo_write = open(request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                sprintf(new_box_name, "/%s", request.box_name);
                box_reponse.code = 6;

                int remove = tfs_unlink(new_box_name);//TODO: Potencialmente mudar o unlink para apagar ficheiro mesmo que tenha leitores a ler
                
                if(remove == -1){
                    box_reponse.return_code = -1;
                    strcpy(box_reponse.error_message, "ganda erro a remover");//TODO
                }else{
                    box_reponse.return_code = 0;
                    strcpy(box_reponse.error_message, "\0");
                    //apagar box da lista
                    delete_box(&box_list, new_box_name);
                }                
                send_box_response(box_reponse, worker_fifo_write);   
                close(worker_fifo_write);         
                break;
            case 7: //Received request for box listing
                remove_strings_from_buffer(request_buffer + offset, listing_request.client_named_pipe_path , sizeof(listing_request.client_named_pipe_path));
                worker_fifo_write = open(listing_request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                box_data = box_list.head;
                while(box_data != NULL){
                    offset = 0;
                    listing_response.code = 8;
                    memcpy(listing_buffer, &listing_response.code, sizeof(listing_response.code));
                    offset += sizeof(listing_response.code);
                    if(box_data == box_list.tail){
                        listing_response.last = 1;
                    }else{
                        listing_response.last = 0;
                    }
                    memcpy(listing_buffer + offset, &listing_response.last, sizeof(listing_response.last));
                    offset += sizeof(listing_response.last);
                    strcpy(box_data->box_name, box_data->box_name + 1);//retirar a / do inicio
                    store_string_in_buffer(listing_buffer + offset, box_data->box_name, sizeof(box_data->box_name));
                    offset += sizeof(box_data->box_name);
                    listing_response.box_size = box_data->box_size;
                    memcpy(listing_buffer + offset, &listing_response.box_size, sizeof(listing_response.box_size));
                    offset += sizeof(listing_response.box_size);
                    listing_response.n_publishers = box_data->n_publishers;
                    memcpy(listing_buffer + offset, &listing_response.n_publishers, sizeof(listing_response.n_publishers));
                    offset += sizeof(listing_response.n_publishers);
                    listing_response.n_subscribers = box_data->n_subscribers;
                    memcpy(listing_buffer + offset, &listing_response.n_subscribers, sizeof(listing_response.n_subscribers));

                    bytes_written = write(worker_fifo_write, listing_buffer, sizeof(listing_buffer));
                    if (bytes_written < 0) {
                        fprintf(stderr, "[ERR]: write failed\n");
                        exit(EXIT_FAILURE);
                    }

                    box_data = box_data->next;
                }
                close(worker_fifo_write);
                break;
            default:
                printf("Received unknown message with code %u\n", code);
                break;
        }
    }
    return NULL;
}
