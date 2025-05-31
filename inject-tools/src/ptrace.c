#include <sys/types.h>
#include <sys/user.h>
#include "ptrace.h"

void ptrace_attach(pid_t target)
{
    int waitpidstatus;

    if(ptrace(PTRACE_ATTACH, target, NULL, NULL) == -1)
    {
        perror("ptrace(PTRACE_ATTACH) failed\n");
        exit(1);
    }

    if(waitpid(target, &waitpidstatus, WUNTRACED) != target)
    {
        fprintf(stderr, "waitpid(%d) failed\n", target);
        exit(1);
    }
}

void ptrace_detach(pid_t target)
{
    if(ptrace(PTRACE_DETACH,target,NULL,NULL) < 0)
    {
        perror("ptrace(PTRACE_DETACH) failed\n");
        exit(1);
    }
}

void ptrace_continue(pid_t target)
{
    if(ptrace(PTRACE_CONT,target,NULL,NULL) < 0)
    {
        perror("ptrace(PTRACE_CONT) failed\n");
        exit(1);
    }
}

void ptrace_getregs(pid_t target, struct user_regs_struct* arm_regs)
{
#if defined(__aarch64__)
    struct iovec regIoVec;
    regIoVec.iov_base = arm_regs;
    regIoVec.iov_len = sizeof(*arm_regs);
    if(ptrace(PTRACE_GETREGSET, target, NT_PRSTATUS, &regIoVec) < 0)
    {
        perror("ptrace(PTRACE_GETREGSET) failed\n");
        disp("io %p,%ld\n",regIoVec.iov_base,regIoVec.iov_len);
        exit(1);
    }
#else
    if(ptrace(PTRACE_GETREGS, target, NULL, arm_regs) == -1)
    {
        perror("ptrace(PTRACE_GETREGS) failed\n");
        exit(1);
    }
#endif
}

void ptrace_setregs(pid_t target, struct user_regs_struct* arm_regs)
{
#if defined(__aarch64__)
    struct iovec regIoVec;
    regIoVec.iov_base = arm_regs;
    regIoVec.iov_len = sizeof(*arm_regs);
    if(ptrace(PTRACE_SETREGSET, target,NT_PRSTATUS, &regIoVec) < 0)
    {
        perror("ptrace(PTRACE_SETREGSET) failed\n");
        disp("io %p,%ld\n",regIoVec.iov_base,regIoVec.iov_len);
        exit(1);
    }
#else
    if(ptrace(PTRACE_SETREGS, target, NULL, arm_regs) == -1)
    {
        perror("ptrace(PTRACE_SETREGS) failed\n");
        exit(1);
    }
#endif
}

int ptrace_readdata(pid_t target_pid,unsigned char* src,unsigned char*buf,size_t size)
{
    long  i = 0,j= 0,remain = 0;
    unsigned char * local_addr = NULL;
    size_t bytes_width = sizeof(long);
    union u 
    {
        long val;
        char chars[bytes_width];
    }d;
    j = size / bytes_width;
    remain = size % bytes_width;
    local_addr = buf;
    for(i = 0;i < j;i++)
    {
        d.val = ptrace(PTRACE_PEEKTEXT,target_pid,src,0);
        memcpy(local_addr,d.chars,bytes_width);
        src += bytes_width;
        local_addr += bytes_width;
    }
    if(remain > 0)
    {
        d.val = ptrace(PTRACE_PEEKTEXT,target_pid,src,0);
        memcpy(local_addr,d.chars,remain);
    }
    return  0;
}

int ptrace_writedata(pid_t target_pid,unsigned char* dest,unsigned char* data,size_t size)
{
    long  i = 0,j= 0,remain = 0;
    unsigned char * local_addr = NULL;
    size_t bytes_width = sizeof(long);
    union u 
    {
        long val;
        char chars[bytes_width];
    }d;
    j = size / bytes_width;
    remain = size % bytes_width;
    local_addr = data;
    for(i = 0;i < j;i++)
    {
        memcpy(d.chars,local_addr,bytes_width);
        ptrace(PTRACE_POKETEXT,target_pid,dest,d.val);
       
        dest += bytes_width;
        local_addr += bytes_width;
    }
    if(remain > 0)
    {
        d.val = ptrace(PTRACE_PEEKTEXT,target_pid,dest,0);
        for(i = 0;i < remain;i++)
        {
            d.chars[i] = *local_addr++;
        }
        ptrace(PTRACE_POKETEXT,target_pid,dest,d.val);
    }
    return  0;
}

unsigned long int ptrace_retval(struct user_regs_struct * regs)
{
#if defined(__arm__) || defined(__aarch64__)
	return regs->ARM_r0;
#elif defined(__i386__)
	return regs->eax;
#else
#error "Not supported"    
#endif
}

unsigned long int ptrace_ip(struct user_regs_struct * regs)
{
#if defined(__arm__) || defined(__aarch64__)
	return regs->ARM_pc;
#elif defined(__i386__)
	return regs->eip;
#else
#error "Not supported"    
#endif
}

static int ptrace_call(pid_t target_pid,unsigned long int  addr,unsigned long int *params,int num_params,struct user_regs_struct * arm_regs)
{
    int i = 0;
    int stat = 0;
#if defined(__arm__)
    int num_param_registers = 4;//r0 -r3
#elif defined(__aarch64__)
    int num_param_registers = 8;//x0-x7
#endif
    for(i = 0;i < num_param_registers && i < num_params;i++)
    {
        arm_regs->regs[i] = params[i];
    }

    if(i < num_params)
    {
        arm_regs->ARM_sp -=(num_params - i) * sizeof(long);
        ptrace_writedata(target_pid,(unsigned char*)arm_regs->sp,(unsigned char*)&params[i],(num_params - i) * sizeof(long));
    }
    arm_regs->ARM_pc = addr;
#if defined(__arm__)
    if(arm_regs->ARM_pc & 0x1)
    {
        /*thumb*/
        arm_regs->ARM_pc &=(~0x1u);
        arm_regs->ARM_cpsr |= CPSR_T_MASK;
    }
    else
    {
        /*arm*/
        arm_regs->ARM_cpsr &= ~CPSR_T_MASK;
    }
#endif    
    arm_regs->ARM_lr = 0;// retun to null,Trigger program to generate SIGSEGV signal
    ptrace_setregs(target_pid,arm_regs); 
    ptrace_continue(target_pid); 
    waitpid(target_pid, &stat, WUNTRACED);
    while (stat != 0xb7f)
    {
        ptrace_continue(target_pid);
        waitpid(target_pid, &stat, WUNTRACED);
    }
    return  0;
}

int ptrace_call_wrapper(pid_t target_pid, const char * func_name, unsigned long int func_addr, unsigned long int* parameters, int param_num, struct user_regs_struct * regs)
{
    disp("Calling %s in target process.\n",func_name);
    if (ptrace_call(target_pid, (unsigned long int)func_addr, parameters, param_num, regs) == -1)
    {
        return -1;
    }
    ptrace_getregs(target_pid, regs);
    disp(COLOR_GREEN"Target process returned from %s, return value=%lx, pc=%lx \n"COLOR_NONE,
                 func_name, ptrace_retval(regs), ptrace_ip(regs));
    return 0;
}

