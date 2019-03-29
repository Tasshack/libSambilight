#define LOG_FILE "/dtv/"LIB_NAME".log"
#define STATIC static

#define _FILE_OFFSET_BITS 64

#ifndef _LARGEFILE64_H
#define _LARGEFILE64_H
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE 1
#endif
#include <sys/types.h>
#include <sys/stat.h>

void LOG(
        const char *fmt, ...)
{
#ifdef LOG_FILE
    va_list ap;

    FILE *f = fopen(LOG_FILE, "a+");
    if(f)
    {
        va_start(ap, fmt);
        vfprintf(f, fmt, ap);
        va_end(ap);

        fflush(f);
        fclose(f);
    }
#endif
}
void vLOG(const char * restrict fmt, va_list ap)
{
#ifdef LOG_FILE
    FILE *f = fopen(LOG_FILE, "a+");
    if(f)
    {
        vfprintf(f, fmt, ap);
        fflush(f);
        fclose(f);
    }
#endif
}
#define log(...) LOG("["LIB_NAME"] "__VA_ARGS__)
#define logh(fmt,...) LOG("["LIB_NAME"] %s, "fmt,__func__+2,__VA_ARGS__)
#define logf(fmt,...) LOG("["LIB_NAME"] %s, "fmt,__func__,__VA_ARGS__)


static void dumpbin(
        const char *path, const void *data, size_t cnt)
{
    FILE *f = fopen(path, "wb+");
    //mylog("test2");
    //mylog("test","test");
    if(f)
    {
        fwrite(data, cnt, 1, f);
        fflush(f);
        fclose(f);
    }
    else
        log("Error saving file '%s'\n", path);
}

#define BX_LR 0xe12fff1e

static void  __attribute__((naked)) bx_lr()
{
    asm volatile ("BX LR");
}
static void  __attribute__((naked)) mov_r0_0_bx_lr()
{
    asm volatile ("MOV R0, #0;"
				   "BX LR");
}
static void  __attribute__((naked)) mov_r0_1_bx_lr()
{
    asm volatile ("MOV R0, #1;"
				   "BX LR");
}
static void  __attribute__((naked)) mov_r0_3_bx_lr()
{
    asm volatile ("MOV R0, #3;"
				   "BX LR");
}


static int PATCH(void *h, void *func, void* patch, int p_size)
{
	int i;
    uint32_t paligned = (uint32_t)func & ~4095;
	uint32_t *p_func = (uint32_t*)func;
	uint32_t *p_patch = (uint32_t*)patch;
    mprotect((uint32_t *)paligned, 8192, PROT_READ | PROT_WRITE | PROT_EXEC);
	for(i=0;i<p_size;i++)
	{
		p_func[i]=p_patch[i];
	}
    mprotect((uint32_t *)paligned, 8192, PROT_READ | PROT_EXEC);
	return 1;
}
static int PATCH_MOV_R0_0_BX_LR(void *h, void *func)
{
	return PATCH(h,func,mov_r0_0_bx_lr,2);
}
static int PATCH_MOV_R0_1_BX_LR(void *h, void *func)
{
	return PATCH(h,func,mov_r0_1_bx_lr,2);
}
static int PATCH_MOV_R0_3_BX_LR(void *h, void *func)
{
	return PATCH(h,func,mov_r0_3_bx_lr,2);
}
static int patch_adbg_CheckSystem(
        void *h)
{
    void *adbg_CheckSystem = dlsym(h, "adbg_CheckSystem");
    if(!adbg_CheckSystem)
        return -1;

    if(*(uint32_t*)adbg_CheckSystem == *(uint32_t*)bx_lr) //orig: 0xE92D4800
        return 1;

	return PATCH(h,adbg_CheckSystem,bx_lr,1);
}
static int getArgCArgV(
        const char *libpath, char **argv)
{
    const int EXTRA_COOKIE = 0x82374021;

    uint32_t argc = 1;
    argv[0] = (char *)libpath;
    void *mem = (void*)(libpath + strlen(libpath) + 1);

    uint32_t aligned = (uint32_t)mem;
    aligned = (aligned + 3) & ~3;

    uint32_t *extra = (uint32_t*)aligned;
    if(extra[0] != EXTRA_COOKIE)
        return 0;

    argc += extra[1];
    uint32_t *_argv = &extra[2];
    for(int i = 0; i < argc; i++)
        argv[i + 1] = (char *)(aligned + _argv[i]);

    return argc;
}

char* getOptArg(char **argv, int argc, char *option)
{
    for(int i=0;i<argc;i++)
    {
        if(strstr(argv[i],option)==argv[i])
        {
            return argv[i]+strlen(option);
        }
    }
    return 0;
}

#define _HOOK_IMPL(F_RET,F, ...) \
    typedef F_RET (*F)(__VA_ARGS__); \
    STATIC sym_hook_t hook_##F; \
    STATIC F_RET x_##F(__VA_ARGS__)


#define _HOOK_DISPATCH(F, ...) \
    hijack_pause(&hook_##F); \
    void *h_ret = (void *) ((F)hook_##F.addr)(__VA_ARGS__); \
    hijack_resume(&hook_##F) 
#define _HOOK_DISPATCH2(F, ...) \
    hijack_pause(&hook_##F); \
    void *h_ret2 = (void *) ((F)hook_##F.addr)(__VA_ARGS__); \
    hijack_resume(&hook_##F) 

#define _HOOK_DISPATCH_LOG(F, ...) \
    log(">>> %s\n", __func__); \
    _HOOK_DISPATCH(F, __VA_ARGS__); \
    log("<<< %s %p\n", __func__, h_ret)

#define cacheflush(from, size)   __clear_cache((void*)from, (void*)((unsigned long)from+size))

typedef struct 
{
    void *fn;
    const char *name;
} dyn_fn_t;



STATIC int dyn_sym_tab_init(
        void *h, dyn_fn_t *fn_tab, uint32_t cnt)
{
    for(int i = 0; i < cnt; i++)
    {
        void *fn = dlsym(h, fn_tab[i].name);
        if(!fn)
        {
            log("dlsym '%s' failed.\n", fn_tab[i].name);
            continue;
            //return -1;
        }

        log("%s [%p].\n", fn_tab[i].name, fn);

        fn_tab[i].fn = fn;
    }
    return 0;
}

void log_buf(char *name, unsigned char *buf)
{
    int i;
    log("%s: ",name);
    for(i=0;i<16;i++)
        LOG("0x%02x ",buf[i]);
    LOG("\n");
}

typedef struct
{
    sym_hook_t *hook;
    dyn_fn_t *dyn_fn;
    void *fnHook;
} hook_entry_t;

STATIC int set_hooks(
        hook_entry_t *hooks, uint32_t cnt)
{
    for(int i = 0; i < cnt; i++)
    {
        void *fn = hooks[i].dyn_fn->fn;
        if(!fn)
            continue;

        if(!hooks[i].fnHook)
            continue;

        uint32_t paligned = (uint32_t)fn & ~4095;
        mprotect((uint32_t *)paligned, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);

        hijack_start(hooks[i].hook, fn, hooks[i].fnHook);
    }
    return 0;
}

STATIC int remove_hooks(
        hook_entry_t *hooks, uint32_t cnt)
{
    for(int i = 0; i < cnt; i++)
    {
        hijack_stop(hooks[i].hook);
    }
}

typedef union
{
    const void *procs[100];
    const char *names[100];
} samyGO_CTX_t;

//STATIC int samyGO_whacky_t_init(void *h, samyGO_whacky_t *ctx, uint32_t cnt)
STATIC int samyGO_whacky_t_init(void *h, void *paramCTX, uint32_t cnt)
{
    samyGO_CTX_t *ctx;
    ctx=paramCTX;
    for(int i = 0; i < cnt ; i++)
    {
        if(!ctx->procs[i])
            continue;

        void *fn = dlsym(h, ctx->procs[i]);
        if(!fn)
            log("dlsym '%s' failed.\n", ctx->procs[i]);
        else
            log("%s [%p].\n",  ctx->procs[i], fn);
        ctx->procs[i] = fn;
    }
    return 0;
}

#include <errno.h>

void *sgo_shmem_open(
        const char *path, size_t size)
{
    char _path[PATH_MAX + 1] = { 0 };
    //mkdir(SAMYGO_RT_DIR, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    strncpy(_path,path,PATH_MAX);

    errno = 0;
    int fd=-1;
    int created = 0, tmp=0;
    if((fd = open(_path, O_RDWR , (mode_t)0600)) > 0)
        created=0;
    else if((fd = open(_path, O_RDWR | O_CREAT, (mode_t)0600)) > 0)
        created=1;
    else
        return 0;

        lseek(fd, size, SEEK_SET);
        tmp=write(fd, "", 1);

        //logf("SHM, created: %d\n",created);

    void *mem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if(!mem)
        return 0;
    if(created)
        memset(mem, 0, size);

    return mem;
}
void *sgo_shmem_init(const char *path, size_t size)
{
    void *shm = sgo_shmem_open(path, size);
    if(!shm)
    {
        logf("Error: shmem open '%s'.\n", path);
        return 0;
    }
    return shm;
}
void sgo_shmem_close(void *mem, size_t size)
{
    size_t aligned = size & ~0xFFF + 0x1000;
    msync(mem, aligned, MS_SYNC);
    munmap(mem, size);
}
