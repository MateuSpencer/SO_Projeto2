#include "logging.h"

int main(int argc, char **argv) {
    if(argc == 3){

        for(int thread = 0; thread < argv[2]; thread++){
            //lançar As threads todas?
        }
        
    }
    fprintf(stderr, "usage: mbroker <register_pipe_name> <max_sessions>\n");
    return -1;
}
