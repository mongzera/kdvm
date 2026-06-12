#ifndef H_VM_HANDLER
#define H_VM_HANDLER

typedef struct {
    void* ctx; // something that describes the device / interface

    int (*read)(void *ctx, void *buf, int n);
    int (*write)(void *ctx, const void *buf, int n);
} VM_Handle;


#endif
