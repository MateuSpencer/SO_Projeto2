#include "protocol.h"

#include <string.h>


void insert_at_beginning(BoxList *list, char* box_name, uint64_t box_size, uint64_t n_publishers, uint64_t n_subscribers) {
    BoxData *new_box_data = (BoxData *)malloc(sizeof(BoxData));
    pthread_mutex_init(&new_box_data->box_condvar_lock, NULL);
    pthread_cond_init(&new_box_data->box_condvar, NULL);
    strcpy(new_box_data->box_name, box_name);
    new_box_data->box_size = box_size;
    new_box_data->n_publishers = n_publishers;
    new_box_data->n_subscribers = n_subscribers;
    new_box_data->next = list->head;
    list->head = new_box_data;
    if(list->tail == NULL) {
        list->tail = new_box_data;
    }
}

BoxData* find_box(BoxData *current, char* box_name) {
    while (current != NULL) {
        if (strcmp(box_name, current->box_name) == 0){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void delete_box(BoxList *list, char* box_name) {
    BoxData  *temp = list->head;
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


ssize_t read_fifo(int fifo, char *buffer, size_t n_bytes){
    ssize_t bytes_read = read(fifo, buffer, n_bytes);
    if (bytes_read == -1) {
        return -1;
    }
    return bytes_read;
}

void store_string_in_buffer(char* buffer, char* str1, size_t space) {
    size_t str1_len = strlen(str1);
    size_t len = str1_len > space ? space : str1_len;//truncar strings demasiado grandes
    memcpy(buffer, str1, len);
    if (str1_len < space) {
        memset(buffer + str1_len, '\0', space - str1_len);
    }
}

void remove_strings_from_buffer(char* buffer, char* str1, size_t space) {
    size_t i;
    for (i = 0; i < space; i++) {
        if (buffer[i] == '\0') {
            break;
        }
        str1[i] = buffer[i];
    }
    str1[i] = '\0';
}

size_t remove_first_string_from_buffer(char* buffer, char* str1, size_t max_space) {
    size_t i;
    for (i = 0; i < max_space; i++) {
        if (buffer[i] == '\0') {
            break;
        }
        str1[i] = buffer[i];
    }
    str1[i] = '\0';
    return i + 1;
}

void send_request(Request request, int fifo){
    long unsigned int offset = 0;
    char request_buffer [sizeof(Request)];
    memcpy(request_buffer, &request.code, sizeof(request.code));
    offset += sizeof(request.code);
    store_string_in_buffer(request_buffer + offset, request.client_named_pipe_path, sizeof(request.client_named_pipe_path));
    offset += sizeof(request.client_named_pipe_path);
    store_string_in_buffer(request_buffer + offset, request.box_name, sizeof(request.box_name));
    // Write the serialized message to the FIFO
    ssize_t bytes_written = write(fifo, request_buffer, sizeof(request_buffer));
    if (bytes_written < 0) {
        fprintf(stderr, "[ERR]: write failed\n");
        exit(EXIT_FAILURE);
    }
}


void send_box_response(Box_Response response, int fifo){
    long unsigned int offset = 0;
    char response_buffer [sizeof(Box_Response)];
    memcpy(response_buffer, &response.code, sizeof(response.code));
    offset += sizeof(response.code);
    memcpy(response_buffer + offset, &response.return_code, sizeof(response.return_code));
    offset += sizeof(response.return_code);
    store_string_in_buffer(response_buffer + offset, response.error_message, sizeof(response.error_message));
    // Write the serialized message to the FIFO
    ssize_t bytes_written = write(fifo, response_buffer, sizeof(response_buffer));
    if (bytes_written < 0) {
        fprintf(stderr, "[ERR]: write failed\n");
        exit(EXIT_FAILURE);
    }
}

BoxData* merge_sort_boxdata_list(BoxData* head) {
    // Base case: empty or single element list
    if (head == NULL || head->next == NULL) {
        return head;
    }

    // Dividir a lista a meio
    BoxData* slow = head;
    BoxData* fast = head->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    BoxData* head2 = slow->next;
    slow->next = NULL;

    // ordenar recursivamente as duas metades
    BoxData* left = merge_sort_boxdata_list(head);
    BoxData* right = merge_sort_boxdata_list(head2);

    // unir as duas metades ordenadas
    BoxData dummy;
    BoxData* tail = &dummy;
    while (left != NULL && right != NULL) {
        if (strcmp(left->box_name, right->box_name) < 0) {
            tail->next = left;
            left = left->next;
        } else {
            tail->next = right;
            right = right->next;
        }
        tail = tail->next;
    }
    if (left != NULL) {
        tail->next = left;
    } else {
        tail->next = right;
    }

    return dummy.next;
}