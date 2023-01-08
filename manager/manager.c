#include "logging.h"

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

int main(int argc, char **argv) {

    if(argc == 3){//list


        return 0;
    }else if(argc == 4){
        if(strcmp(argv[2],"create")){



            return 0;
        }else if(strcmp(argv[2],"remove")){
            


            return 0;
        }
    }
    print_usage();
    return -1;
}
