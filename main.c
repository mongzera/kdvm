#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Guard POSIX headers for desktop builds only
#if !defined(__arm__) && !defined(__embedded__) && !defined(PICO_BOARD)
    #include <unistd.h> // Required for readlink()
#else
    #include "pico/stdlib.h" // Required for stdio_init_all() on RP2040
#endif

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

// Dynamically gets the directory where the grrvm binary lives
void get_executable_dir(char* buffer, size_t size) {
    if (!buffer || size == 0) return;

#if defined(__arm__) || defined(__embedded__) || defined(PICO_BOARD)
    // Bare-metal / Embedded Target (RP2040, Cortex-M)
    strncpy(buffer, ".", size - 1);
    buffer[size - 1] = '\0';
#else
    // Linux / POSIX PC Target
    ssize_t count = readlink("/proc/self/exe", buffer, size - 1);
    if (count != -1) {
        buffer[count] = '\0';
        char* last_slash = strrchr(buffer, '/');
        if (last_slash) {
            *last_slash = '\0'; // Truncate filename to leave directory path
        }
    } else {
        strncpy(buffer, ".", size - 1); // Fallback to current directory
        buffer[size - 1] = '\0';
    }
#endif
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

    // Execute
    printf("--- Execution Started ---\n");
    clock_t start = clock();
    vm_execute(my_machine);

    printf("--- Execution Halted ---\n");
    clock_t end = clock();

    // Calculate elapsed time (fix unit print to seconds)
    double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Execution time: %f seconds\n", cpu_time_used);

    free(my_machine);
}

void command_compile_vm(int argc, char* argv[]) {
#if defined(__arm__) || defined(__embedded__) || defined(PICO_BOARD)
    printf("[ERROR] Cannot invoke Java assembler on bare-metal microcontroller.\n");
    return;
#else
    if (argc < 3) {
        printf("[ERROR] Missing input file for compilation.\n");
        print_help(argc, argv);
        return;
    }

    const char* input_file = argv[2];
    char output_file[256];
    derive_output_filename(input_file, output_file, sizeof(output_file));

    char exe_dir[512];
    get_executable_dir(exe_dir, sizeof(exe_dir));

    char assembler_path[1024];
    snprintf(assembler_path, sizeof(assembler_path), "%s/assembler/Assembler.java", exe_dir);

    char command[2048];
    snprintf(command, sizeof(command), "java \"%s\" \"%s\" \"%s\"", assembler_path, input_file, output_file);

    printf("[COMPILING] Running: %s\n", command);

    int result = system(command);
    if (result == 0) {
        print_done("Compilation complete.");
    } else {
        printf("[ERROR] Assembler failed with exit code %d\n", result);
    }
#endif
}

int main(int argc, char* argv[]) {
#if defined(__arm__) || defined(__embedded__) || defined(PICO_BOARD)
    stdio_init_all();
    sleep_ms(2000); // Give serial monitor 2s to attach

    printf("\n--- Starting GRRVM on Pico ---\n");

    while (true) {
        printf("Hello\n");
    }
    return 0;

#else
    // Desktop CLI check
    if (argc < 2) {
        print_help(argc, argv);
        return 1;
    }
#endif

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
