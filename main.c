#include <stdio.h>
#include "core/grrvm.h"
#include "assembler/assember.h"
#include <stdlib.h>
#include <time.h>

typedef enum Commands {
    COMMAND_COMPILE = 0x0,
    COMMAND_RUN
};

void print_unknown_command(int argc, char* argv[]){
    printf("Unknown Command: %s\n", argv[1]);
    printf("Idk ts, man, type properly\n");
}

void print_help(int argc, char* argv[]){
    printf("Help!!!\n");
    printf("Usage: %s -r, --run <program.kdo> to run objects.\n", argv[0]);
    printf("Usage: %s -c, --help <program.kdm> to compile to objects.\n", argv[0]);
}

void print_done(const char* msg){
    printf("[DONE] %s\n", msg);
}

void print_running(const char* msg){
    printf("[RUNNING] %s\n", msg);
}

void command_run_vm(char* filename){
    // Initialize a Single VM Instance for now

    print_running("VM Initialization...");

    VM *my_machine = (VM*)malloc(sizeof(VM));
    vm_init(my_machine);

    print_done("VM Initialization...");

    // Load the program into this specific instance
    print_running("KDM File Loading...");
    if (load_kdm_file(filename, my_machine) != 0) return;
    print_done("KDM File Loading...");
    struct timespec start, end;

    // Execute
    printf("--- Execution Started ---\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    vm_execute(my_machine);
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("--- Execution Halted ---\n");

    // Calculate elapsed time
    long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);

    printf("Execution time: %lld nanoseconds\n", elapsed_ns);
}

int command = -1;

int main(int argc, char* argv[]) {

    if (argc < 2) {
        print_help(argc, argv);
        return 1;
    }


    if(strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--run") == 0) command = COMMAND_RUN;
    else if(strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--compile") == 0) command = COMMAND_RUN;
    else if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) print_help(argc, argv);
    else print_unknown_command(argc, argv);

    if(command == -1) return 1;
    if(command == COMMAND_RUN) command_run_vm(argv[2]);

    return 0;
}
