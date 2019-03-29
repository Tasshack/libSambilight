/* 
 *  bugficks
 *	(c) 2013
 *
 *  License: GPLv3
 *
 */
//////////////////////////////////////////////////////////////////////////////

#ifndef __COMMOM_H__
#define __COMMOM_H__

//////////////////////////////////////////////////////////////////////////////

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stddef.h>
#include <limits.h>
#include <stdint.h>

//////////////////////////////////////////////////////////////////////////////

#define ARRAYSIZE(AR) (sizeof(AR)/sizeof(AR[0]))
#ifdef __cplusplus
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#else    
#define C_ASSERT(e)  char __C_ASSERT__[(e)?1:-1] __attribute__((unused)); 
#endif    
//#define C_ASSERT(e) do{extern char __C_ASSERT__[(e)?1:-1];(void)__C_ASSERT__;}while(0)

#define FIELD_OFFSET(s, field)  ((ptrdiff_t)&((s *)(0))->field)

//////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus

#define EXTERN_C        extern "C"
#define EXTERN_C_BEGIN  EXTERN_C {
#define EXTERN_C_END    }

#else // ! #ifdef __cplusplus

#define EXTERN_C
#define EXTERN_C_BEGIN
#define EXTERN_C_END

#endif // #ifdef __cplusplus

//////////////////////////////////////////////////////////////////////////////

#define DECL_SPEC(...) __attribute__((__VA_ARGS__))
 
//////////////////////////////////////////////////////////////////////////////

#endif // #ifndef __COMMOM_H__

//////////////////////////////////////////////////////////////////////////////
