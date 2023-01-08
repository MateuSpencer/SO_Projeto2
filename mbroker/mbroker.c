#include "logging.h"

int main(int argc, char **argv) {
    if(argc == 3){

        //criar register pipe

        /*for(int thread = 0; thread < argv[2]; thread++){
            //lançar As threads todas? - Começar so com uma
        }*/
        
    }
    fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions>\n");
    return -1;
}
