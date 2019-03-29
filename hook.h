#ifndef __HOOK_H__
#define __HOOK_H__


#include "common.h"


#define HIJACK_SIZE 12

typedef struct
{
    void *addr;
    unsigned char o_code[HIJACK_SIZE];
    unsigned char n_code[HIJACK_SIZE];
} sym_hook_t;


EXTERN_C_BEGIN

void hijack_start(
        sym_hook_t *sa, void *target, void *_new);

void hijack_pause(
        sym_hook_t *sa);

void hijack_resume(
        sym_hook_t *sa);

void hijack_stop(
        sym_hook_t *sa);

EXTERN_C_END

#endif //__HOOK_H__
