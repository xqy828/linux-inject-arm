#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    printf("usage: %s [-n process-name] [-p pid] [library-to-inject]\n", name);
}

int getProcessExebyPid(char * outExe,unsigned int pid)
{
    char exe[32] = {0};
    int linken = 0;
    char linkPath[MAX_STR] = {0};

    if(outExe == NULL)
    {
        printf("outExe is null \r\n");
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
        printf("readlink %s failed (errno:%d:%s)\r\n",exe,errno,strerror(errno));
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
        printf("name is null \r\n");
        return -1;
    }
    if(pid == NULL)
    {
        printf("pid is null \r\n");
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
        printf("open %s is failed, %s\r\n",filename,strerror(errno));
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
    unsigned long int add = 0;
    void* selfhandle = NULL; 
    void* funcAddr  = NULL;
    selfhandle = dlopen("libc.so.6", RTLD_LAZY);
    if(selfhandle == NULL)
    {
        printf("Error loading libc.so.6: %s\n",dlerror());
        return  0;
    }
	funcAddr = dlsym(selfhandle, funcName);
    if(funcAddr == NULL)
    {
        printf("Error finding cos: %s\n",dlerror());
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
        printf("get local libc addr failed \r\n");
        return 1;
    }
    targetProcessLibcAddr = getLibcBaseAddr(pid,library_name);
    if(targetProcessLibcAddr == 0)
    {
        printf("get target process libc addr failed \r\n");
        return 1;
    }
    funcAddrOffset = localFuncAddr - localLibcAddr;
    targetProcessLibcFuncAddr = targetProcessLibcAddr + funcAddrOffset;
    printf("[+]:%s,local %s[%lx], target %s[%lx],target LibcFuncAddr[%lx]\n", 
        __func__,library_name,localLibcAddr, library_name,targetProcessLibcAddr,targetProcessLibcFuncAddr);
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
    unsigned long int parameters[10];
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

    long sohandle = 0;
    unsigned char errorbuf[128] = {0};
    unsigned char* errorrc = NULL;
    unsigned long int hook_entry_addr = 0;
    struct user_regs_struct regs, original_regs;
    printf("[+]:%s Injecting process: %d\n",__func__,target_pid);

    mmap_addr = getTargetProcessLibcFuncAddr(target_pid,libcname,(unsigned long int)mmap);
    printf("[+]:%s Target process mmap address: %lx\n", __func__,mmap_addr);
    
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

    local_dlopenAddr = getLibcFuncAddr("__libc_dlopen_mode");
    local_dlsymAddr = getLibcFuncAddr("__libc_dlsym");
    local_dlcloseAddr = getLibcFuncAddr("__libc_dlclose");
    local_dlerrorAddr = getLibcFuncAddr("__dlerror");

    dlopenAddr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_dlopenAddr);
    dlsymAddr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_dlsymAddr);
    dlcloseAddr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_dlcloseAddr);
    dlerrorAddr = getTargetProcessLibcFuncAddr(target_pid,libcname,local_dlerrorAddr);
    printf("[+]:%s Get imports: dlopen: %lx, dlsym: %lx, dlclose: %lx, dlerror: %lx\n",
                __func__,dlopenAddr, dlsymAddr, dlcloseAddr, dlerrorAddr);
    printf("library path = %s\n", InjectlibPath);
    ptrace_writedata(target_pid, (unsigned char*)map_base, (unsigned char*)InjectlibPath, strlen(InjectlibPath) + 1);

    parameters[0] = map_base;
    parameters[1] = RTLD_NOW| RTLD_GLOBAL;

    if (ptrace_call_wrapper(target_pid, "__libc_dlopen_mode", dlopenAddr, parameters, 2, &regs) == -1)
    {
        return -1;
    }

    sohandle = ptrace_retval(&regs);
    if(sohandle == 0)
    {
        if(ptrace_call_wrapper(target_pid,"__dlerror",dlerrorAddr,0,0,&regs)== -1)
        {
            return  -1;
        }
        ptrace_readdata(target_pid,errorrc,errorbuf,128);
    }


#define FUNCTION_NAME_ADDR_OFFSET       0x100
    ptrace_writedata(target_pid, (unsigned char *)(map_base + FUNCTION_NAME_ADDR_OFFSET), (unsigned char*)function_name, strlen(function_name) + 1);
    parameters[0] = sohandle;
    parameters[1] = map_base + FUNCTION_NAME_ADDR_OFFSET;

    if (ptrace_call_wrapper(target_pid, "__libc_dlsym", dlsymAddr, parameters, 2, &regs) == -1)
    {
        return -1;
    }
    hook_entry_addr = ptrace_retval(&regs);
    printf("hook_entry_addr = %lx\n", hook_entry_addr);

#define FUNCTION_PARAM_ADDR_OFFSET      0x200
    ptrace_writedata(target_pid, (unsigned char *)(map_base + FUNCTION_PARAM_ADDR_OFFSET), (unsigned char*)params, strlen(params) + 1);
    parameters[0] = map_base + FUNCTION_PARAM_ADDR_OFFSET;

    if (ptrace_call_wrapper(target_pid,function_name, hook_entry_addr, parameters, 1, &regs) == -1)
    {
        return  -1;
    }

    printf("Press enter to dlclose and detach\n");
    getchar();
    parameters[0] = sohandle;

    if (ptrace_call_wrapper(target_pid, "__libc_dlclose", dlcloseAddr, parameters, 1, &regs) == -1)
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

        printf("targeting process \"%s\" with pid %d\n", TargetProcessName, TargetProcessPid);
    }
    else if(!strcmp(command, "-p"))
    {
        TargetProcessPid = atoi(commandArg);
        printf("targeting process with pid %d\n", TargetProcessPid);
    }
    else
    {
        usage(argv[0]);
        return 1;
    }

    rc = injectProcess(TargetProcessPid,libc_name,InjectlibPath,"hook_entry","hello hook");

    return  0;
}





























