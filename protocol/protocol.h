#ifndef __protocol__
#define __protocol__

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct  {
    uint8_t code;
    char client_named_pipe_path[256];
    char box_name[32];
}Request;
typedef struct  {
    uint8_t code;
    char client_named_pipe_path[256];
}ListingRequest;
typedef struct  {
    uint8_t code;
    uint8_t last;
    char box_name[32];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
}ListingResponse;

typedef struct  {
    uint8_t code;
    char message[1024];
}Message;

typedef struct  {
    uint8_t code;
    int32_t return_code;
    char error_message[1024];
}Box_Response;

//Struct for box data
typedef struct boxdata{
    char box_name[32];
    uint64_t box_size;
    uint64_t n_publishers;
    uint64_t n_subscribers;
    pthread_mutex_t box_condvar_lock;
    pthread_cond_t box_condvar;
    struct boxdata *next;
}BoxData;
//Struct to support list of boxes
typedef struct{
    BoxData *head;
    BoxData *tail;
    pthread_mutex_t box_list_lock;
}BoxList;

void insert_at_beginning(BoxList *list, char* box_name, uint64_t box_size, uint64_t n_publishers, uint64_t n_subscribers);

BoxData* find_box(BoxData *current, char* box_name);

void delete_box(BoxList *list, char* box_name);

ssize_t read_fifo(int fifo, char *buffer, size_t n_bytes);

void store_string_in_buffer(char* buffer, char* str1, size_t space);

void remove_strings_from_buffer(char* buffer, char* str1, size_t space);

size_t remove_first_string_from_buffer(char* buffer, char* str1, size_t max_space);

void send_request(Request request, int fifo);

void send_box_response(Box_Response reponse, int fifo);

#endif