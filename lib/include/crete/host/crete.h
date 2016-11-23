#ifndef CRETE_HOST_CRETE_H
#define CRETE_HOST_CRETE_H

#include <crete/custom_opcode.h>

inline
// for memory that needs to be touched.
static void __crete_touch_buffer(volatile void *buffer, unsigned size)
{
    unsigned i;
    volatile char *b = (volatile char *) buffer;
    for (i = 0; i < size; ++i) {
        *b; ++b;
    }
}

inline
void crete_send_concolic_name(const char* name)
{
    size_t name_size = strlen(name);
    __crete_touch_buffer((void*)name, name_size);

    __asm__ __volatile__(
            CRETE_INSTR_SEND_CONCOLIC_NAME()
        : : "a" (name), "c" (name_size)
    );
}

inline
// Inject call to helper_crete_make_symbolic, so that klee can catch
void __crete_make_concolic_internal(void)
{
    __asm__ __volatile__(
        CRETE_INSTR_MAKE_SYMBOLIC()
    );
}

inline
void crete_make_concolic(void* addr,
                         size_t size,
                         const char* name)
{
    // CRETE_INSTR_SEND_CONCOLIC_NAME must be sent before CRETE_INSTR_MAKE_CONCOLIC,
    // to satisfy the hanlder's order in qemu/runtime-dump/custom-instruction.cpp
    crete_send_concolic_name(name);

    __crete_touch_buffer(addr, size);

    __asm__ __volatile__(
            CRETE_INSTR_MAKE_CONCOLIC()
        : : "a" (addr), "c" (size)
    );

    __crete_make_concolic_internal();
}

#endif // CRETE_HOST_CRETE_H
