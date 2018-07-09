#ifndef CRETE_HOST_CRETE_H
#define CRETE_HOST_CRETE_H

#include <stdint.h>
#include <crete/custom_opcode.h>

typedef UINT64 size_t;

static inline
// for memory that needs to be touched.
void __crete_touch_buffer(volatile void *buffer, unsigned size)
{
    unsigned i;
    volatile char *b = (volatile char *) buffer;
    for (i = 0; i < size; ++i) {
        *b; ++b;
    }
}

static inline
void crete_send_concolic_name(const char* name)
{
//    size_t name_size = strlen(name);
    size_t name_size = AsciiStrLen(name); // Apparently EDK2 in non-optimized mode doesn't understand strlen().
    __crete_touch_buffer((void*)name, name_size);

    __asm__ __volatile__(
            CRETE_INSTR_SEND_CONCOLIC_NAME()
        : : "a" (name), "c" (name_size)
    );
}

static inline
// Inject call to helper_crete_make_symbolic, so that klee can catch
void __crete_make_concolic_internal(void)
{
    __asm__ __volatile__(
        CRETE_INSTR_MAKE_SYMBOLIC()
    );
}

static inline
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

static inline
void crete_assume_begin()
{
    __asm__ __volatile__(
            CRETE_INSTR_ASSUME_BEGIN()
    );
}

static inline
void crete_assume_(int cond)
{
    __asm__ __volatile__(
        CRETE_INSTR_ASSUME()
        : : "a" (cond)
    );
}


#define crete_assume(cond) \
    crete_assume_begin(); \
    crete_assume_(cond)

static inline
void crete_capture_begin(void)
{
    __asm__ __volatile__(
            CRETE_INSTR_CAPTURE_BEGIN()
    );
}

static inline
void crete_capture_end(void)
{
    __asm__ __volatile__(
            CRETE_INSTR_CAPTURE_END()
    );
}

static inline
void crete_prime(void)
{
    __asm__ __volatile__(
            CRETE_INSTR_PRIME()
    );
}

static inline
void crete_dump(void)
{
    __asm__ __volatile__(
            CRETE_INSTR_DUMP()
    );
}

static inline
void crete_insert_instr_read_port(void* addr, size_t size)
{
//    __crete_touch_buffer((void *)addr, size);

    __asm__ __volatile__(
        CRETE_INSTR_READ_PORT()
        : : "a" (addr), "c" (size)
    );
}

static inline
void crete_pause_for_savevm(void)
{
    // Utilize read_port for the purpose of pausing the program until
    // the user has saved a snapshot, and send it off to crete-dispatch.
    unsigned short port = 0;
    do
    {
        crete_insert_instr_read_port((void*)&port,
                                     (size_t)sizeof(port));

    }while(port == 0);
}

#endif // CRETE_HOST_CRETE_H