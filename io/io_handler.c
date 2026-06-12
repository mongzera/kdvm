#include "io_handler.h"
#include <stdio.h>

int write_terminal(void *ctx, void *buf, int n){
    return fwrite(buf, 1, n, stdout);
}

int read_terminal(void *ctx, void *buf, int n) {
    (void)ctx;

    int i;
    char *cbuf = (char *)buf;

    for (i = 0; i < n; i++) {
        int c = getchar();

        if (c == EOF) {
            break;
        }

        cbuf[i] = (char)c;
    }

    return i; // number of bytes actually read
}
