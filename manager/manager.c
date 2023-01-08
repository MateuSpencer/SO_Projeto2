#include "logging.h"

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

int main(int argc, char **argv) {

    if(argc == 3){//list
    //Pedido de listagem de caixas:
        //[ code = 7 (uint8_t) ] | [ client_named_pipe_path (char[256]) ]

        return 0;
    }else if(argc == 4){
        if(strcmp(argv[2],"create")){

        //mandar pedido para criar uma caixa
            //[ code = 3 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
            return 0;
        }else if(strcmp(argv[2],"remove")){
            
            //mandar pedido para iniciar sessao e apagar uma caixa
                //[ code = 5 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]

            return 0;
        }
    }
    print_usage();
    return -1;
}
