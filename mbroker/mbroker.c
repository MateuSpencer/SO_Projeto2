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

//volatile sig_atomic_t to make it thread safe
volatile sig_atomic_t stop_workers = 0;

//Struct for box data
typedef struct {
    char box_name[32];
    int n_publishers;
    int n_subscribers;
    pthread_cond_t box_new_message_cond;
    struct node *next;
}BoxData;
//Struct to support list of boxes
typedef struct{
    struct node *head;
    struct node *tail;
}BoxList;

void insert_at_beginning(BoxList *list, char* box_name, int n_publishers, int n_subscribers) {
    BoxData *new_box_data = (BoxData*)malloc(sizeof(BoxData));
    pthread_cond_init(&new_box_data->box_new_message_cond, NULL);
    strcpy(new_box_data->box_name, box_name);
    new_box_data->n_publishers = n_publishers;
    new_box_data->n_subscribers = n_subscribers;
    new_box_data->next = list->head;
    list->head = new_box_data;
    if(list->tail == NULL) {
        list->tail = new_box_data;
    }
}

void delete_node(BoxList *list, char* box_name) {
    BoxData *temp = list->head;
    BoxData *prev = NULL;
    //node to be deleted is the head
    if (temp != NULL && strcmp(box_name, temp->box_name) == 0) {
        list->head = temp->next;
        free(temp);
        return;
    }
    //node to be deleted is not the head
    while (temp != NULL && strcmp(box_name, temp->box_name) != 0) {
        prev = temp;
        temp = temp->next;
    }
    //the node was not found
    if (temp == NULL) {
        return;
    }
    // Unlink the node from the list
    prev->next = temp->next;
    if(temp == list->tail) list->tail = prev; 
    free(temp);
}


void *worker_thread_func(void *arg);

int main(int argc, char **argv){

    if(argc == 3){
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
    uint8_t code = 0;
    Request request;
    Box_Response box_reponse;
    Message message;
    ListingRequest listing_request;
    char message_buffer[sizeof(Message)];
    char new_box_name[40];
    int worker_fifo_write;
    int worker_fifo_read;

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
                if(publisher_file_handle >= 0){//ir logo para o close e dar SIGPIPE
                    bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));// para o teste caso nao tenha closed, ignorar o 0 mandado
                    bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));
                    while(bytes_read > 0){
                        memcpy(&message.code, message_buffer, sizeof(message.code));
                        offset = 0;
                        offset += sizeof(message.code);
                        remove_strings_from_buffer(message_buffer + offset, message.message , sizeof(message.message));
                        printf("--%s--\n",message.message);//TODO
                        tfs_write(publisher_file_handle, message.message, sizeof(message.message));
                        //ler proxima mensagem
                        bytes_read = read(worker_fifo_read, message_buffer, sizeof(message_buffer));
                    }
                    if (bytes_read < 0){//error
                        fprintf(stderr, "[ERR]: read failed1\n");
                        exit(EXIT_FAILURE);
                    }
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
                
                //le da caixa e envia estas
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
                }                
                send_box_response(box_reponse, worker_fifo_write);
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
                }                
                send_box_response(box_reponse, worker_fifo_write);            
                break;
            case 7: //Received request for box listing
                remove_strings_from_buffer(request_buffer + offset, listing_request.client_named_pipe_path , sizeof(listing_request.client_named_pipe_path));
                worker_fifo_write = open(listing_request.client_named_pipe_path, O_WRONLY);
                if (worker_fifo_write == -1){
                    fprintf(stderr, "[ERR]: open failed\n");
                    exit(EXIT_FAILURE);
                }
                    //[ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
            
                break;
            default:
                printf("Received unknown message with code %u\n", code);
                break;
        }
    }
    return NULL;
}
