#include "logging.h"

int main(int argc, char **argv) {
    if(argc == 3){

        //criar register pipe

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
