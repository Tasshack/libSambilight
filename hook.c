#include "hook.h"
#include <string.h>

#define cacheflush(from, size)   __clear_cache((void*)from, (void*)((unsigned long)from+size))

void hijack_start(
       sym_hook_t *sa, void *target, void *_new)
{
    unsigned char o_code[HIJACK_SIZE], n_code[HIJACK_SIZE];

    if ( (unsigned long)target % 4 == 0 )
    {
        // ldr pc, [pc, #0]; .long addr; .long addr
        memcpy(n_code, "\x00\xf0\x9f\xe5\x00\x00\x00\x00\x00\x00\x00\x00", HIJACK_SIZE);
        *(unsigned long *)&n_code[4] = (unsigned long)_new;
        *(unsigned long *)&n_code[8] = (unsigned long)_new;
    }
    else // Thumb
    {
        // add r0, pc, #4; ldr r0, [r0, #0]; mov pc, r0; mov pc, r0; .long addr
        memcpy(n_code, "\x01\xa0\x00\x68\x87\x46\x87\x46\x00\x00\x00\x00", HIJACK_SIZE);
        *(unsigned long *)&n_code[8] = (unsigned long)_new;
        target--;
    }

    #if __DEBUG__
    printf("Hooking function 0x%p with 0x%p\n", target, _new);
    #endif

    memcpy(o_code, target, HIJACK_SIZE);

    memcpy(target, n_code, HIJACK_SIZE);
    cacheflush(target, HIJACK_SIZE);

    sa->addr = target;
    memcpy(sa->o_code, o_code, HIJACK_SIZE);
    memcpy(sa->n_code, n_code, HIJACK_SIZE);

}

void hijack_pause(
       sym_hook_t *sa)
{
    #if __DEBUG__
    printf("Pausing function hook 0x%p\n", target);
    #endif

    memcpy(sa->addr, sa->o_code, HIJACK_SIZE);
    cacheflush(sa->addr, HIJACK_SIZE);
}

void hijack_resume(
       sym_hook_t *sa)
{
    #if __DEBUG__
    printf("Resuming function hook 0x%p\n", target);
    #endif

    memcpy(sa->addr, sa->n_code, HIJACK_SIZE);
    cacheflush(sa->addr, HIJACK_SIZE);
}

void hijack_stop(
       sym_hook_t *sa)
{
    #if __DEBUG__
    printf("Unhooking function 0x%p\n", target);
    #endif

    memcpy(sa->addr, sa->o_code, HIJACK_SIZE);
    cacheflush(sa->addr, HIJACK_SIZE);
}
