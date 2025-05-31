#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <features.h>
#include <gnu/libc-version.h>
#include <sys/user.h>
#include <wait.h>
#include <elf.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "ptrace.h"

#define MAX_STR   (1024)
#define libc_name  "libc.so.6"

extern void *__libc_dlopen_mode  (const char *__name, int __mode);
extern void *__libc_dlsym   (void *__map, const char *__name);
extern int   __libc_dlclose (void *__map);
extern char *__dlerror (void);

static void usage(char* name)
{
    disp("usage: %s [-n process-name] [-p pid] [library-to-inject]\n", name);
}

int compare_glibc_versions(int major1, int minor1, int major2, int minor2) 
{
    if (major1 != major2) 
    {
        return major1 - major2;
    }
    return minor1 - minor2;
}

int getProcessExebyPid(char * outExe,unsigned int pid)
{
    char exe[32] = {0};
    int linken = 0;
    char linkPath[MAX_STR] = {0};

    if(outExe == NULL)
    {
        disp("outExe is null \r\n");
        return -1;
    }
    if(pid == 0)
    {
        snprintf(exe,sizeof(exe),"/proc/self/exe");
    }
    else
    {
        snprintf(exe,sizeof(exe),"/proc/%u/exe",pid);
    }
    linken = (int)readlink(exe,linkPath,(size_t)sizeof(linkPath)-1);
    if(linken < 0)
    {
        disp("readlink %s failed (errno:%d:%s)\r\n",exe,errno,strerror(errno));
        return -1;
    }
    else
    {
        linkPath[linken] = '\0';
        snprintf(outExe,MAX_STR,"%s",linkPath);
    }
    return 0;
}

int getProcessPidbyName(const char *name,int *pid)
{
    DIR* dir = NULL;
    struct dirent *procDirs;
    int id = -1;
    char* exeName = NULL;
    char* exePath = NULL;
    int linken = 0,len = 0;
    char linkPath[MAX_STR] = {0};
    char* exeToken = NULL;
    if(name == NULL)
    {
        disp("name is null \r\n");
        return -1;
    }
    if(pid == NULL)
    {
        disp("pid is null \r\n");
        return -1;
    }
    dir = opendir("/proc");
    if (dir == NULL)
    {
        return -1;
    }
    while ((procDirs = readdir(dir)) != NULL)
    {
        if (procDirs->d_type != DT_DIR)
        {
            continue;
        }
        id = atoi(procDirs->d_name);
        len = 10 + strlen(procDirs->d_name) + 1;// /proc/%s/exe
        exePath = calloc(len,sizeof(char));
        if(exePath == NULL)
        {
            continue;
        }
        sprintf(exePath, "/proc/%s/exe", procDirs->d_name);
        exePath[len-1] = '\0';
        linken = (int)readlink(exePath, linkPath, MAX_STR - 1);
        if(linken == -1)
        {
            free(exePath);
            continue;
        }
        linkPath[linken] = '\0';

        exeToken = strtok(linkPath, "/");
        while(exeToken)
        {
            exeName = exeToken;
            exeToken = strtok(NULL, "/");
        }

        if(strcmp(exeName, name) == 0)
        {
            free(exePath);
            closedir(dir);
            *pid = id;
            return 0;
        }
        free(exePath);
    }
    closedir(dir);
    return -1;
}

int CheckTargetProcessloaded(pid_t pid, const char* library_name)
{
    FILE *fp;
    char filename[30];
    char line[MAX_STR];
    sprintf(filename, "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if(fp == NULL)
    {
        exit(1);
    }
    while(fgets(line,MAX_STR, fp) != NULL)
    {
        if(strstr(line, library_name) != NULL)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}


unsigned long int getLibcBaseAddr(pid_t pid,const char* library_name)
{
    unsigned long int start_addr = 0,end_addr = 0;
    unsigned long int  base_addr = 0;
    FILE *fp = NULL;
    char filename[30];
    char line[MAX_STR];

    if (pid < 0) 
    {
        /* self process */
        snprintf(filename, sizeof(filename), "/proc/self/maps");
    } 
    else 
    {
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }

    fp = fopen(filename, "r");
    if(fp == NULL)
    {
        disp("open %s is failed, %s\r\n",filename,strerror(errno));
        return 0;
    }

    while(fgets(line, MAX_STR, fp) != NULL)
    {
        if (strstr(line, library_name) && strstr(line,"r-xp"))
        {
            if(sscanf(line,"%lx-%lx",&start_addr,&end_addr) == 2)
            {
                base_addr = start_addr;
                break;
            }
        }
    }
    fclose(fp) ;
    return base_addr;
}

unsigned long int getLibcFuncAddr(char* funcName)
{
    void* selfhandle = NULL; 
    void* funcAddr  = NULL;
    //selfhandle = dlopen("libc.so.6", RTLD_LAZY);
    selfhandle = dlopen(libc_name, RTLD_LAZY);
    if(selfhandle == NULL)
    {
        disp("Error loading libc.so.6: %s\n",dlerror());
        return  0;
    }
    funcAddr = dlsym(selfhandle, funcName);
    if(funcAddr == NULL)
    {
        disp("Error finding funcName:%s,%s\n",funcName,dlerror());
        return 0;
    }
    return (unsigned long int)funcAddr;

}

unsigned long int  getTargetProcessLibcFuncAddr(pid_t pid,const char* library_name,unsigned long int localFuncAddr)
{
    unsigned long int localLibcAddr = 0;
    unsigned long int funcAddrOffset = 0;
    unsigned long int targetProcessLibcAddr = 0;
    unsigned long int targetProcessLibcFuncAddr = 0;
    localLibcAddr = getLibcBaseAddr(-1,library_name);
    if(localLibcAddr == 0)
    {
        disp("get local libc addr failed \r\n");
        return 1;
    }
    targetProcessLibcAddr = getLibcBaseAddr(pid,library_name);
    if(targetProcessLibcAddr == 0)
    {
        disp("get target process libc addr failed \r\n");
        return 1;
    }
    funcAddrOffset = localFuncAddr - localLibcAddr;
    targetProcessLibcFuncAddr = targetProcessLibcAddr + funcAddrOffset;
    disp(COLOR_GREEN"local %s[0x%lx], target %s[0x%lx],funcAddrOffset %ld target LibcFuncAddr[0x%lx]\n"COLOR_NONE, 
        library_name,localLibcAddr, library_name,targetProcessLibcAddr,funcAddrOffset,targetProcessLibcFuncAddr);
    return targetProcessLibcFuncAddr;
}

/**********************************************************************
targetpid: target task pid
libcname: glibc name  libc.so.6
InjectlibPath: dynamic-link library want to inject
function_name: name of the function to be called in the InjectlibPath
params:Inputs for function_name calls in InjectlibPath
************************************************************************/
int injectProcess(pid_t target_pid,const char* libcname,const char* InjectlibPath,const char* function_name,const char* params)
{
    unsigned long int parameters[10] = {0};
    unsigned long int mmap_addr = 0;
    unsigned long int map_base = 0;
    unsigned long int dlopenAddr = 0;
    unsigned long int dlsymAddr = 0;
    unsigned long int dlcloseAddr = 0;
    unsigned long int dlerrorAddr = 0;

    unsigned long int local_dlopenAddr = 0;
    unsigned long int local_dlsymAddr = 0;
    unsigned long int local_dlcloseAddr = 0;
    unsigned long int local_dlerrorAddr = 0;
    unsigned long int local_mmapAddr = 0;

    long sohandle = 0;
    unsigned char errorbuf[128] = {0};
    unsigned char* errorrc = NULL;
    unsigned long int hook_entry_addr = 0;
    struct user_regs_struct regs, original_regs;

    int major = 0;
    int minor = 0;
    int micro = 0;
    const char *version_str = gnu_get_libc_version();

    memset(&regs,0,sizeof(regs));
    memset(&original_regs,0,sizeof(regs));
    
    disp("Injecting process: %d\n",target_pid);
    local_mmapAddr = getLibcFuncAddr("mmap");
    mmap_addr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_mmapAddr);
    disp(COLOR_GREEN"Target process mmap address: 0x%lx\n"COLOR_NONE,mmap_addr);
    
   if (sscanf(version_str, "%d.%d.%d", &major, &minor, &micro) < 2) 
   {
        if (sscanf(version_str, "%d.%d", &major, &minor) < 2) 
        {
            disp("Error: Failed to parse glibc version\n");
            return -1;
        }
    }

    disp(COLOR_GREEN"Running GLIBC version: %d.%d.%d\n"COLOR_NONE, major, minor, micro);
/********************************************************************************************
GLIBC 2.34 Major new features:
  In order to support smoother in-place-upgrades and to simplify
  the implementation of the runtime all functionality formerly
  implemented in the libraries libpthread, libdl, libutil, libanl has
  been integrated into libc.  New applications do not need to link with
  -lpthread, -ldl, -lutil, -lanl anymore.  For backwards compatibility,
  empty static archives libpthread.a, libdl.a, libutil.a, libanl.a are
  provided, so that the linker options keep working.  Applications which
  have been linked against glibc 2.33 or earlier continue to load the
  corresponding shared objects (which are now empty).  The integration
  of those libraries into libc means that additional symbols become
  available by default.  This can cause applications that contain weak
  references to take unexpected code paths that would only have been
  used in previous glibc versions when e.g. preloading libpthread.so.0,
  potentially exposing application bugs.
**********************************************************************************************/
    if(compare_glibc_versions(major,minor,2,34) < 0)
    {
        local_dlopenAddr = getLibcFuncAddr("__libc_dlopen_mode");
        local_dlsymAddr = getLibcFuncAddr("__libc_dlsym");
        local_dlcloseAddr = getLibcFuncAddr("__libc_dlclose");
        local_dlerrorAddr = getLibcFuncAddr("__dlerror");
        if((local_dlopenAddr == 0)|| 
            (local_dlsymAddr == 0) ||
            (local_dlcloseAddr == 0) ||
            (local_dlerrorAddr ==0))
        {
            disp("get local libc func addr failed\n");
            return -1;
        }

    }
    else
/****************************************************************************************
    __libc_dlopen_mode,__libc_dlsym,__libc_dlclose,__dlerror,was removed from glibc 2.34

commit 7a5db2e82fbb6c3a6e3fdae02b7166c5d0e8c7a8
Author: Florian Weimer <fweimer@redhat.com>
Date:   Wed Jul 7 08:40:41 2021 +0200

    elf: Clean up GLIBC_PRIVATE exports of internal libdl symbols

    They are no longer needed after everything has been moved into
    libc.  The _dl_vsym test has to be removed because the symbol
    cannot be used outside libc anymore.

    Reviewed-by: Adhemerval Zanella  <adhemerval.zanella@linaro.org>

commit 466c1ea15f461edb8e3ffaf5d86d708876343bbf
Author: Florian Weimer <fweimer@redhat.com>
Date:   Thu Jun 3 08:26:04 2021 +0200

    dlfcn: Rework static dlopen hooks

    Consolidate all hooks structures into a single one.  There are
    no static dlopen ABI concerns because glibc 2.34 already comes
    with substantial ABI-incompatible changes in this area.  (Static
    dlopen requires the exact same dynamic glibc version that was used
    for static linking.)

    The new approach uses a pointer to the hooks structure into
    _rtld_global_ro and initalizes it in __rtld_static_init.  This avoids
    a back-and-forth with various callback functions.

    Reviewed-by: Adhemerval Zanella  <adhemerval.zanella@linaro.org>
****************************************************************************************/
    {
        local_dlopenAddr =getLibcFuncAddr("dlopen");
        local_dlsymAddr = getLibcFuncAddr("dlsym");
        local_dlcloseAddr = getLibcFuncAddr("dlclose");
        local_dlerrorAddr = getLibcFuncAddr("dlerror");
        if((local_dlopenAddr == 0)|| 
            (local_dlsymAddr == 0) ||
            (local_dlcloseAddr == 0) ||
            (local_dlerrorAddr ==0))
        {
            disp("get local libc func addr failed\n");
            return -1;
        }
    }
    dlopenAddr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_dlopenAddr);
    dlsymAddr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_dlsymAddr);
    dlcloseAddr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_dlcloseAddr);
    dlerrorAddr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_dlerrorAddr);
    disp(COLOR_GREEN"Get imports: dlopen: 0x%lx, dlsym: 0x%lx, dlclose: 0x%lx, dlerror: 0x%lx\n"COLOR_NONE,dlopenAddr, dlsymAddr, dlcloseAddr, dlerrorAddr);
    disp("library path = %s\n",InjectlibPath);
    
    ptrace_attach(target_pid);
    ptrace_getregs(target_pid,&regs);
    memcpy(&original_regs, &regs, sizeof(regs));// back up regs


    /* call mmap */
    parameters[0] = 0;  // addr
    parameters[1] = 0x4000; // size
    parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;  // prot
    parameters[3] =  MAP_ANONYMOUS | MAP_PRIVATE; // flags
    parameters[4] = 0; //fd
    parameters[5] = 0; //offset
    if (ptrace_call_wrapper(target_pid, "mmap", mmap_addr, parameters, 6, &regs) == -1)
    {
        return -1;
    }
    map_base = ptrace_retval(&regs);
    disp(COLOR_GREEN"Target process after mmap map_base = 0x%lx\n"COLOR_NONE,map_base);
    ptrace_writedata(target_pid, (unsigned char*)map_base, (unsigned char*)InjectlibPath, strlen(InjectlibPath) + 1);

    parameters[0] = map_base;
    parameters[1] = RTLD_NOW|RTLD_GLOBAL;

    if (ptrace_call_wrapper(target_pid, "__libc_dlopen_mode/dlopen", dlopenAddr, parameters, 2, &regs) == -1)
    {
        return -1;
    }

    sohandle = ptrace_retval(&regs);
    if(sohandle == 0)
    {
        if(ptrace_call_wrapper(target_pid,"__dlerror/dlerror",dlerrorAddr,0,0,&regs)== -1)
        {
            return  -1;
        }
        errorrc = (unsigned char*)ptrace_retval(&regs);
        ptrace_readdata(target_pid,errorrc,errorbuf,128);
        disp("Target process open library %s failed, error:%s \n",InjectlibPath,errorbuf);
        ptrace_setregs(target_pid, &original_regs);
        ptrace_detach(target_pid);
        return -1;
    }

    if(CheckTargetProcessloaded(target_pid, InjectlibPath))
    {
        disp("\"%s\" successfully injected\n",InjectlibPath);
    }
    else
    {
        disp("could not inject \"%s\"\n",InjectlibPath);
        ptrace_setregs(target_pid, &original_regs);
        ptrace_detach(target_pid);
        return -1;
    }

#define FUNCTION_NAME_ADDR_OFFSET       0x100
    ptrace_writedata(target_pid, (unsigned char *)(map_base + FUNCTION_NAME_ADDR_OFFSET), (unsigned char*)function_name, strlen(function_name) + 1);
    parameters[0] = sohandle;
    parameters[1] = map_base + FUNCTION_NAME_ADDR_OFFSET;

    if (ptrace_call_wrapper(target_pid, "__libc_dlsym/dlsym", dlsymAddr, parameters, 2, &regs) == -1)
    {
        return -1;
    }
    hook_entry_addr = ptrace_retval(&regs);
    if(hook_entry_addr == 0)
    {
         if(ptrace_call_wrapper(target_pid,"__dlerror/dlerror",dlerrorAddr,0,0,&regs)== -1)
        {
            return  -1;
        }
        errorrc = (unsigned char*)ptrace_retval(&regs);
        ptrace_readdata(target_pid,errorrc,errorbuf,128);
        disp("Target process can't find %s function, in %s,  error:%s\n",function_name,InjectlibPath,errorbuf);
        ptrace_setregs(target_pid, &original_regs);
        ptrace_detach(target_pid);
        return -1;
    }
    disp(COLOR_GREEN"hook_entry_addr = %lx\n"COLOR_NONE, hook_entry_addr);

#define FUNCTION_PARAM_ADDR_OFFSET      0x200
    ptrace_writedata(target_pid, (unsigned char *)(map_base + FUNCTION_PARAM_ADDR_OFFSET), (unsigned char*)params, strlen(params) + 1);
    parameters[0] = map_base + FUNCTION_PARAM_ADDR_OFFSET;

    if (ptrace_call_wrapper(target_pid,function_name, hook_entry_addr, parameters, 1, &regs) == -1)
    {
        return  -1;
    }

    disp("Press enter to dlclose and detach\n");
    getchar();
    parameters[0] = sohandle;

    if (ptrace_call_wrapper(target_pid, "__libc_dlclose/dlclose", dlcloseAddr, parameters, 1, &regs) == -1)
    {
        return -1;
    }

    /* restore */
    ptrace_setregs(target_pid, &original_regs);
    ptrace_detach(target_pid);
    return  0;
}

int main(int argc, char** argv)
{
    char* command = NULL;
    char* commandArg = NULL;
    char* InjectlibName =NULL;
    char* InjectlibPath = NULL;
    char* TargetProcessName = NULL;
    pid_t TargetProcessPid = -1;
    int rc = -1;
    if(argc < 4)
    {
        usage(argv[0]);
        return 1;
    }
    command = argv[1];
    commandArg = argv[2];
    InjectlibName = argv[3];
    InjectlibPath = realpath(InjectlibName, NULL);

    disp(COLOR_GREEN"Compiled with glibc: %d.%d\n"COLOR_NONE, __GLIBC__, __GLIBC_MINOR__);
    disp(COLOR_GREEN"Running on glibc: %s\n"COLOR_NONE, gnu_get_libc_version());

    if(InjectlibPath == NULL)
    {
        fprintf(stderr, "can't find file \"%s\"\n", InjectlibName);
        return 1;
    }

    if(!strcmp(command, "-n"))
    {
        TargetProcessName = commandArg;
        rc = getProcessPidbyName(TargetProcessName,&TargetProcessPid);
        if(TargetProcessPid == -1 || rc != 0)
        {
            fprintf(stderr, "doesn't look like a process named \"%s\" is running right now\n", TargetProcessName);
            return 1;
        }

        disp(COLOR_GREEN"Targeting process \"%s\" with pid %d\n"COLOR_NONE, TargetProcessName, TargetProcessPid);
    }
    else if(!strcmp(command, "-p"))
    {
        TargetProcessPid = atoi(commandArg);
        disp(COLOR_GREEN"Target process pid %d\n"COLOR_NONE,TargetProcessPid);
    }
    else
    {
        usage(argv[0]);
        return 1;
    }

    disp(COLOR_GREEN"Inject library %s\n"COLOR_NONE,InjectlibPath);
    rc = injectProcess(TargetProcessPid,libc_name,InjectlibPath,"hook_entry","hello hook");

    return  0;
}





























