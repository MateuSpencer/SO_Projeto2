#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "../fs/operations.h"
#include "logging.h"
#include "../protocol/protocol.h"
#include "producer-consumer.h"


void *worker_thread_func(void *arg);

//Global variable for list of boxes
BoxList box_list;

int end;//declared globally so that the signal treatment can exit the loop and consequently the program

void sigint_handler(int signum) {
    (void)signum;
    end = 1;
}

int main(int argc, char **argv){

    if(argc == 3){
        if (signal(SIGINT, sigint_handler) == SIG_ERR) {
            exit(EXIT_FAILURE);
        }
        end = 0;
        //Initialize list
        box_list.head = NULL;
        box_list.tail = NULL;
        pthread_mutex_init(&box_list.box_list_lock, NULL);
        char register_pipe_name [strlen(argv[1]) + 3];
        sprintf(register_pipe_name, "../%s", argv[1]);
        //Initialize TFS
        assert(tfs_init(NULL) != -1);
        //Initialize queue
        pc_queue_t queue;
        int num_threads = atoi(argv[2]);
        if (pcq_create(&queue, (size_t)(2*num_threads)) != 0){
        fprintf(stderr, "Error creating producer-consumer queue\n");
        return 1;
        }
        //Check if pipe_name already exists
        if(access(register_pipe_name, F_OK) == 0) {
            fprintf(stderr, "[ERR]: %s pipe_name alreay in use\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        //criar register fifo
        if (mkfifo(register_pipe_name, 0640) != 0) {
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
        int register_fifo_read = open(register_pipe_name, O_RDONLY);
        if (register_fifo_read == -1 && end == 0){
            fprintf(stderr, "[ERR]: open failed\n");
            exit(EXIT_FAILURE);
        }
        if(end == 0){
            //open one end with a ghost writer so it always has a writer
            int register_fifo_ghost_writer = open(register_pipe_name, O_WRONLY);
            if (register_fifo_read == -1){
                fprintf(stderr, "[ERR]: open failed\n");
                exit(EXIT_FAILURE);
            }
            register_fifo_ghost_writer++;//Unused Warning
            register_fifo_ghost_writer--;
            //recieve requests and put them in the queue
            char buffer[sizeof(Request)];
            ssize_t bytes_read = 0;
            
            while(end == 0){
                bytes_read = read(register_fifo_read, buffer, sizeof(buffer));
                if (pcq_enqueue(&queue, buffer) != 0) {
                    fprintf(stderr, "Error enqueuing request\n");
                    return 1;
                }
            }
            bytes_read++;//Unused
            close(register_fifo_ghost_writer);
        }
        close(register_fifo_read);
        if(unlink(register_pipe_name) == -1) {
            fprintf(stderr, "[ERR]: unlink(%s) failed\n", register_pipe_name);
        }
        //Make the worker threads terminate
        //??
        /*// Wait for the worker threads to complete
        for (int i = 0; i < num_threads; i++) {
            pthread_cond_broadcast(&queue.pcq_popper_condvar);
            if (pthread_join(worker_threads[i], NULL) != 0) {
                fprintf(stderr, "Error waiting for worker thread\n");
                return 1;
            }
        }*/
        //free(worker_threads);
        // Destroy the producer-consumer queue
        
        /*if (pcq_destroy(&queue) != 0) {
            fprintf(stderr, "Error destroying producer-consumer queue\n");
            return 1;
        }*/
        tfs_destroy();
        pthread_mutex_destroy(&box_list.box_list_lock);
        return 0;
    }

    fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions>\n");
    return -1;
}

void sigpipe_handler(int signum) {
    (void)signum;
}

void *worker_thread_func(void *arg) {
    pc_queue_t *queue = (pc_queue_t*)arg;
    long unsigned int offset = 0;
    size_t offset_aux;
    size_t prev_offset_aux;
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
    char full_box_buffer[1024];

    while (1) {
        // Dequeue a request
        char *request_buffer = (char*)pcq_dequeue(queue);
        if(request_buffer == NULL)break;
        offset = 0;
        //ler so o primerio byte
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
                //encontar a box pedida
                pthread_mutex_lock(&box_list.box_list_lock);
                box_data = find_box(box_list.head, new_box_name);
                pthread_mutex_unlock(&box_list.box_list_lock);
                if(box_data != NULL && publisher_file_handle >= 0 && box_data->n_publishers == 0 ){//ir logo para o close para dar SIGPIPE
                    box_data->n_publishers = 1;
                    bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));// para o teste caso nao tenha closed, ignorar o 0 mandado
                    bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));
                    while(bytes_read > 0){//escrever as mensagens qu evem pelo pipe
                        memcpy(&message.code, message_buffer, sizeof(message.code));
                        offset = 0;
                        offset += sizeof(message.code);
                        remove_strings_from_buffer(message_buffer + offset, message.message , sizeof(message.message));
                        pthread_mutex_lock(&box_data->box_condvar_lock);
                        bytes_written = tfs_write(publisher_file_handle, message.message, (strlen(message.message) + 1));// escrever um \0 a seguir? ou ja vem com isso? ou nem preciso pq vou sempre ler o mm tamanho
                        if(bytes_written < 0){
                            fprintf(stderr, "[ERR]: TFS write failed\n");
                            break;
                        }
                        if(bytes_written == 0){
                            fprintf(stderr, "BOX %s FULL\n",new_box_name);
                            break;
                        }
                        //aumentar tamanho da caixa
                        size_t message_len = strlen(message.message);
                        box_data->box_size += message_len;
                        pthread_cond_broadcast(&box_data->box_condvar);
                        pthread_mutex_unlock(&box_data->box_condvar_lock);
                        //ler proxima mensagem
                        bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));
                    }
                    if (bytes_read < 0){//error
                        fprintf(stderr, "[ERR]: read failed\n");
                        exit(EXIT_FAILURE);
                    }
                    box_data->n_publishers = 0;
                }
                close(worker_fifo_read);
                break;
            case 2: //Received request for subscriber registration
                if (signal(SIGPIPE, sigpipe_handler) == SIG_ERR) {
                    exit(EXIT_FAILURE);
                }
                remove_strings_from_buffer(request_buffer + offset, request.client_named_pipe_path , sizeof(request.client_named_pipe_path));
                offset += sizeof(request.client_named_pipe_path);
                remove_strings_from_buffer(request_buffer + offset, request.box_name , sizeof(request.box_name));
                
                worker_fifo_write = open(request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                //adicionar / ao inicio do nome da caixa para encontrar
                sprintf(new_box_name, "/%s", request.box_name);
                //obter caixa pedida
                pthread_mutex_lock(&box_list.box_list_lock);
                box_data = find_box(box_list.head, new_box_name);
                pthread_mutex_unlock(&box_list.box_list_lock);
                int subscriber_file_handle = tfs_open(new_box_name, 0);
                if(box_data != NULL && subscriber_file_handle > 0){
                    char message_test[] = "test";
                    bytes_written = write(worker_fifo_write, message_test, sizeof(message_test));//send a test message for the client to know if it was accepted in the box
                    box_data->n_subscribers++;
                    //ler as mensagens que ja estao na caixa
                    bytes_read = tfs_read(subscriber_file_handle, full_box_buffer, sizeof(full_box_buffer));
                    if (bytes_read < 0){//error
                            fprintf(stderr, "[ERR]: TFS read failed\n");
                            exit(EXIT_FAILURE);
                    }
                    message.code = 10;
                    memcpy(message_buffer, &message.code, sizeof(message.code));
                    offset = sizeof(message.code);
                    offset_aux = 0;
                    while(offset_aux <= sizeof(full_box_buffer)){
                        prev_offset_aux = offset_aux; 
                        offset_aux += remove_first_string_from_buffer(full_box_buffer + offset_aux, message.message, (sizeof(full_box_buffer) - offset));
                        if(offset_aux - prev_offset_aux == 1)break;
                        store_string_in_buffer(message_buffer + offset, message.message, sizeof(message.message));
                        bytes_written = write(worker_fifo_write, message_buffer, sizeof(message_buffer));
                    }
                    
                    //Ficar a espera de mensagens
                    uint64_t last_size = box_data->box_size;
                    bytes_written = 1;//para entrar no loop
                    while(bytes_written > 0){//Ficar a espera de mensagens, sair quando der erro a tentar escrevr uma mensagem para o cliente (?? preciso que seja escrito algo na box)
                        pthread_mutex_lock(&box_data->box_condvar_lock);
                        while(box_data->box_size == last_size){
                            pthread_cond_wait(&box_data->box_condvar, &box_data->box_condvar_lock);
                        }
                        bytes_read = tfs_read(subscriber_file_handle, full_box_buffer, sizeof(full_box_buffer));
                        if (bytes_read < 0){//error
                            fprintf(stderr, "[ERR]: TFS read failed\n");
                            exit(EXIT_FAILURE);
                        }
                        last_size = box_data->box_size;
                        pthread_mutex_unlock(&box_data->box_condvar_lock);
                        //serialize message and send
                        remove_first_string_from_buffer(full_box_buffer + (offset-1), message.message, (sizeof(full_box_buffer) - offset));
                        store_string_in_buffer(message_buffer + offset, message.message, sizeof(message.message));
                        bytes_written = 0;
                        bytes_written = write(worker_fifo_write, message_buffer, sizeof(message_buffer));
                    }
                    box_data->n_subscribers = box_data->n_subscribers - 1;
                }
                close(worker_fifo_write);
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
                    strcpy(box_reponse.error_message, "Erro a criar");
                }else{
                    box_reponse.return_code = 0;
                    strcpy(box_reponse.error_message, "\0");
                    //adicionar box a lista
                    pthread_mutex_lock(&box_list.box_list_lock);
                    insert_at_beginning(&box_list, new_box_name, 0, 0, 0);
                    pthread_mutex_unlock(&box_list.box_list_lock);
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

                int remove = tfs_unlink(new_box_name);
                if(remove == -1){
                    box_reponse.return_code = -1;
                    strcpy(box_reponse.error_message, "Erro a remover");
                }else{
                    box_reponse.return_code = 0;
                    strcpy(box_reponse.error_message, "\0");
                    //apagar box da lista
                    pthread_mutex_lock(&box_list.box_list_lock);
                    delete_box(&box_list, new_box_name);
                    pthread_mutex_unlock(&box_list.box_list_lock);
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
                pthread_mutex_lock(&box_list.box_list_lock);//bloquear lista para garantir que nao se altera durante a listagem
                box_data = box_list.head;
                while(box_data != NULL){//iterar por todas as caixas da lista
                    //adquirir todas as informa????es e serializa-las
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
                    store_string_in_buffer(listing_buffer + offset, box_data->box_name + 1, sizeof(box_data->box_name));
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

                    box_data = box_data->next;//passar para proxima caixa
                }
                pthread_mutex_unlock(&box_list.box_list_lock);
                close(worker_fifo_write);
                break;
            default:
                printf("Received unknown message with code %u\n", code);
                break;
        }
    }
    return NULL;
}
