#include <stdio.h>
#include <stdlib.h>
#include <asm/user.h>
#include <asm/ptrace.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>


#if __REG_DOC__
/*regs armv8a & armv7a*/
==============armv7a=============
struct pt_regs {
        unsigned long uregs[18];
};

#define S_R0 0 /* offsetof(struct pt_regs, ARM_r0) */
#define S_R1 4 /* offsetof(struct pt_regs, ARM_r1) */
#define S_R2 8 /* offsetof(struct pt_regs, ARM_r2) */
#define S_R3 12 /* offsetof(struct pt_regs, ARM_r3) */
#define S_R4 16 /* offsetof(struct pt_regs, ARM_r4) */
#define S_R5 20 /* offsetof(struct pt_regs, ARM_r5) */
#define S_R6 24 /* offsetof(struct pt_regs, ARM_r6) */
#define S_R7 28 /* offsetof(struct pt_regs, ARM_r7) */
#define S_R8 32 /* offsetof(struct pt_regs, ARM_r8) */
#define S_R9 36 /* offsetof(struct pt_regs, ARM_r9) */
#define S_R10 40 /* offsetof(struct pt_regs, ARM_r10) */
#define S_FP 44 /* offsetof(struct pt_regs, ARM_fp) */
#define S_IP 48 /* offsetof(struct pt_regs, ARM_ip) */
#define S_SP 52 /* offsetof(struct pt_regs, ARM_sp) */
#define S_LR 56 /* offsetof(struct pt_regs, ARM_lr) */
#define S_PC 60 /* offsetof(struct pt_regs, ARM_pc) */
#define S_PSR 64 /* offsetof(struct pt_regs, ARM_cpsr) */
#define S_OLD_R0 68 /* offsetof(struct pt_regs, ARM_ORIG_r0) */
#define PT_REGS_SIZE 72 /* sizeof(struct pt_regs) */
#define SVC_DACR 72 /* offsetof(struct svc_pt_regs, dacr) */
#define SVC_REGS_SIZE 76 /* sizeof(struct svc_pt_regs) */

==============armv8a=============

struct pt_regs {
        union {
                struct user_pt_regs user_regs;
                struct {
                        u64 regs[31];
                        u64 sp;
                        u64 pc;
                        u64 pstate;
                };
        };
        u64 orig_x0;
#ifdef __AARCH64EB__
        u32 unused2;
        s32 syscallno;
#else
        s32 syscallno;
        u32 unused2;
#endif
        u64 sdei_ttbr1;
        /* Only valid when ARM64_HAS_GIC_PRIO_MASKING is enabled. */
        u64 pmr_save;
        u64 stackframe[2];

        /* Only valid for some EL1 exceptions. */
        u64 lockdep_hardirqs;
        u64 exit_rcu;
};

#define S_X0 0 /* offsetof(struct pt_regs, regs[0]) */
#define S_X2 16 /* offsetof(struct pt_regs, regs[2]) */
#define S_X4 32 /* offsetof(struct pt_regs, regs[4]) */
#define S_X6 48 /* offsetof(struct pt_regs, regs[6]) */
#define S_X8 64 /* offsetof(struct pt_regs, regs[8]) */
#define S_X10 80 /* offsetof(struct pt_regs, regs[10]) */
#define S_X12 96 /* offsetof(struct pt_regs, regs[12]) */
#define S_X14 112 /* offsetof(struct pt_regs, regs[14]) */
#define S_X16 128 /* offsetof(struct pt_regs, regs[16]) */
#define S_X18 144 /* offsetof(struct pt_regs, regs[18]) */
#define S_X20 160 /* offsetof(struct pt_regs, regs[20]) */
#define S_X22 176 /* offsetof(struct pt_regs, regs[22]) */
#define S_X24 192 /* offsetof(struct pt_regs, regs[24]) */
#define S_X26 208 /* offsetof(struct pt_regs, regs[26]) */
#define S_X28 224 /* offsetof(struct pt_regs, regs[28]) */
#define S_FP 232 /* offsetof(struct pt_regs, regs[29]) */
#define S_LR 240 /* offsetof(struct pt_regs, regs[30]) */
#define S_SP 248 /* offsetof(struct pt_regs, sp) */
#define S_PC 256 /* offsetof(struct pt_regs, pc) */
#define S_PSTATE 264 /* offsetof(struct pt_regs, pstate) */
#define S_SYSCALLNO 280 /* offsetof(struct pt_regs, syscallno) */
#define S_SDEI_TTBR1 288 /* offsetof(struct pt_regs, sdei_ttbr1) */
#define S_PMR_SAVE 296 /* offsetof(struct pt_regs, pmr_save) */
#define S_STACKFRAME 304 /* offsetof(struct pt_regs, stackframe) */
#define PT_REGS_SIZE 336 /* sizeof(struct pt_regs) */

#endif

#if defined(__aarch64__)//armv7a compatible with armv8a architecture
#define pt_regs   user_pt_regs
#define uregs     regs
#define ARM_pc    pc
#define ARM_sp    sp
#define ARM_cpsr  pstate
#define ARM_r0    regs[0]
#define ARM_lr    regs[30]
#endif


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

void ptrace_getregs(pid_t target, struct pt_regs* arm_regs)
{
#if defined(__aarch64__)
    int local_register = NT_PRSTATUS; 
    struct iovec regIoVec;
    regIoVec.iov_base = arm_regs;
    regIoVec.iov_len = sizeof(*arm_regs);
    if(ptrace(PTRACE_GETREGSET, target, (void*)local_register, &regIoVec) < 0)
    {
        perror("ptrace(PTRACE_GETREGSET) failed\n");
        printf("io %llx,%d",regIoVec.iov_base,regIoVec.iov_len);
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

void ptrace_setregs(pid_t target, struct pt_regs* arm_regs)
{
#if defined(__aarch64__)
    int local_register = NT_PRSTATUS; 
    struct iovec regIoVec;
    regIoVec.iov_base = arm_regs;
    regIoVec.iov_len = sizeof(*arm_regs);
    if(ptrace(PTRACE_SETREGSET, target, (void*)local_register, &regIoVec) < 0)
    {
        perror("ptrace(PTRACE_SETREGSET) failed\n");
        printf("io %llx,%d",regIoVec.iov_base,regIoVec.iov_len);
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

static int ptrace_readdata(pid_t target_pid,unsigned char* src,unsigned char*buf,size_t size)
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

static int ptrace_writedata(pid_t target_pid,unsigned char* dest,unsigned char* data,size_t size)
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
        d.val = ptrace(PTRACE_POKETEXT,target_pid,dest,0);
       
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

static int ptrace_call(pid_t target_pid,unsigned  long int  addr,long *params,int num_params,struct pt_regs * arm_regs)
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
    arm_regs->ARM_lr = 0;// retun to null,Trigger program to generate SIGSEGV signal
    if (ptrace_setregs(pid, arm_regs) == -1 || ptrace_continue(pid) == -1) 
    {
        printf("error\n");
        return -1;
    }
    waitpid(pid, &stat, WUNTRACED);
    while (stat != 0xb7f)
    {
        if (ptrace_continue(pid) == -1) 
        {
            printf("error\n");  
            return -1;  
        }
        waitpid(pid, &stat, WUNTRACED);
    }
    return  0;
}

void ptrace_call_wrapper(pid_t target_pid, const char * func_name, (unsigned long int)func_addr, long * parameters, int param_num, struct pt_regs * regs)
{
    printf("[+]:%s Calling %s in target process.\n", __func__,func_name);
    if (ptrace_call(target_pid, (unsigned long int)func_addr, parameters, param_num, regs) == -1)
    {
        exit(1);
    }
    if (ptrace_getregs(target_pid, regs) == -1)
    {
        exit(1);
    }
    printf("[+] Target process returned from %s, return value=%x, pc=%x \n",
                func_name, ptrace_retval(regs), ptrace_ip(regs));
}

long ptrace_retval(struct pt_regs * regs)
{
#if defined(__arm__) || defined(__aarch64__)
	return regs->ARM_r0;
#elif defined(__i386__)
	return regs->eax;
#else
#error "Not supported"    
#endif
}

long ptrace_ip(struct pt_regs * regs)
{
#if defined(__arm__) || defined(__aarch64__)
	return regs->ARM_pc;
#elif defined(__i386__)
	return regs->eip;
#else
#error "Not supported"    
#endif
}









