#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "logging.h"
#include "producer-consumer.h"

int main(int argc, char **argv) {
    if(argc == 3){

    
    
        // Remove pipe if it does not exist
        if (unlink(argv[1]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    //criar register fifo: S_IWUSR(read permision for owner), S_IWOTH(write permisiions for others)
    if (mkfifo(argv[1], S_IRUSR | S_IWOTH) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Open pipe for writing
    // This waits for someone to open it for reading
    int tx = open(argv[1], O_WRONLY);
    if (tx == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

        /*for(int thread = 0; thread < argv[2]; thread++){
            //lançar As threads todas? - Começar so com uma
        }*/
        

        //loop que retira mensagens do registerfifo e as processa - No futuro será a fila producer consumer
            //chama função para processar mensagem e essa função depois mand iso para uma thread?

            //Resposta ao pedido de criação de caixa:[ code = 4 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
                //return ée 0 ou -1
                    //se for erro envia mensagem de erro, senao inicializa com \0
            //Resposta ao pedido de remoção de caixa:[ code = 6 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
    
    
            //A resposta à listagem de caixas vem em várias mensagens, do seguinte tipo:
                //[ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
                //last é um se for a ultima da listagem, senao 0
                //box size é o tamanho em bytes da caixa
    
    
    
            //O servidor envia mensagens para o subscritor do tipo:[ code = 10 (uint8_t) ] | [ message (char[1024]) ]
    
    
    
    }
    fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions>\n");
    return -1;
}
