#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // Required for readlink()
#include "core/grrvm.h"

typedef enum Commands {
    COMMAND_COMPILE = 0x0,
    COMMAND_RUN
} Commands;

void print_unknown_command(int argc, char* argv[]) {
    printf("Unknown Command: %s\n", argv[1]);
    printf("Type '%s -h' for usage instructions.\n", argv[0]);
}

void print_help(int argc, char* argv[]) {
    printf("Help!!!\n");
    printf("Usage: %s -h, --help Prints this text!.\n", argv[0]);
    printf("Usage: %s -r, --run <program.grro> to run objects.\n", argv[0]);
    printf("Usage: %s -c, --compile <program.grr> to compile to objects.\n", argv[0]);
}

void print_done(const char* msg) {
    printf("[DONE] %s\n", msg);
}

void print_running(const char* msg) {
    printf("[RUNNING] %s\n", msg);
}

// Dynamically gets the directory where the grrvm binary actually lives
void get_executable_dir(char* buffer, size_t size) {
    ssize_t count = readlink("/proc/self/exe", buffer, size - 1);
    if (count != -1) {
        buffer[count] = '\0';
        char* last_slash = strrchr(buffer, '/');
        if (last_slash) {
            *last_slash = '\0'; // Truncate filename to leave directory path
        }
    } else {
        strncpy(buffer, ".", size); // Fallback to current directory
    }
}

void derive_output_filename(const char* input, char* output, size_t max_len) {
    strncpy(output, input, max_len - 1);
    output[max_len - 1] = '\0';

    // Strip trailing .grr if present, then attach .grro
    char *dot = strrchr(output, '.');
    if (dot && strcmp(dot, ".grr") == 0) {
        *dot = '\0';
    }
    strncat(output, ".grro", max_len - strlen(output) - 1);
}

void command_run_vm(const char* filename) {
    print_running("VM Initialization...");

    VM *my_machine = (VM*)malloc(sizeof(VM));
    if (!my_machine) {
        printf("[ERROR] Failed to allocate memory for VM.\n");
        return;
    }
    vm_init(my_machine);

    print_done("VM Initialization...");

    // Load the program into this specific instance
    print_running("KDM File Loading...");
    if (load_kdm_file(filename, my_machine) != 0) {
        free(my_machine);
        return;
    }
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

    free(my_machine);
}

void command_compile_vm(int argc, char* argv[]) {
    if (argc < 3) {
        printf("[ERROR] Missing input file for compilation.\n");
        print_help(argc, argv);
        return;
    }

    const char* input_file = argv[2];
    char output_file[256];
    derive_output_filename(input_file, output_file, sizeof(output_file));

    // 1. Get the directory where grrvm binary resides
    char exe_dir[512];
    get_executable_dir(exe_dir, sizeof(exe_dir));

    // 2. Build absolute path to assembler/assembler.py relative to binary location
    char assembler_path[1024];
    snprintf(assembler_path, sizeof(assembler_path), "%s/assembler/assembler.py", exe_dir);

    // 3. Construct full shell command
    char command[2048];
    snprintf(command, sizeof(command), "python3 \"%s\" \"%s\" \"%s\"", assembler_path, input_file, output_file);

    printf("[COMPILING] Running: %s\n", command);

    int result = system(command);
    if (result == 0) {
        print_done("Compilation complete.");
    } else {
        printf("[ERROR] Assembler failed with exit code %d\n", result);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argc, argv);
        return 1;
    }

    int command = -1;

    if (strcmp(argv[1], "-r") == 0 || strcmp(argv[1], "--run") == 0) {
        command = COMMAND_RUN;
    } else if (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--compile") == 0) {
        command = COMMAND_COMPILE;
    } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help(argc, argv);
        return 0;
    } else {
        print_unknown_command(argc, argv);
        return 1;
    }

    if (command == COMMAND_RUN) {
        if (argc < 3) {
            printf("[ERROR] Missing .grro object file to run.\n");
            print_help(argc, argv);
            return 1;
        }
        command_run_vm(argv[2]);
    } else if (command == COMMAND_COMPILE) {
        command_compile_vm(argc, argv);
    }

    return 0;
}
