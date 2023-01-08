#include "logging.h"

int main(int argc, char **argv) {
    int msgCtr = 0;

    if(argc == 4){

        //enviar pelo argv[1] (resgiter pipe) um pedido para se subscrever à caixa argv[3], e dar o nome do fifo entre a thred e este processo argv[2]
            //[ code = 2 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
        //le as mensagens que ja estao na caixa

        //fica a espera  de novas mensagens serem escritas
            //nao espera ativa
            //fprintf(stdout, "%s\n", message);

        //deve processar o SIGINT
            //fechar sessão
            //escrever numero de mensagens recebidas


    }
    
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
    return -1;
}
