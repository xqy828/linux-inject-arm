
#ifndef PTRACE_H
#define PTRACE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <elf.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <unistd.h> 
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

typedef struct user_regs_struct user_regs_struct;

#define COLOR_NONE  "\033[0m"
#define COLOR_RED   "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_BLUE  "\033[1;34m"
#define UNUSED_PARA(para)   ((void)(para))
#define disp(format, ...)  \
do{\
    printf("[\033[0;31m-[+]-\033[0m]:%s-(%05d)]"format,__FUNCTION__,__LINE__,##__VA_ARGS__);\
}while(0)

#if defined __REG_DOC__
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

=============================GLIBC AARCH64=========================
struct user_regs_struct
{
  unsigned long long regs[31];
  unsigned long long sp;
  unsigned long long pc;
  unsigned long long pstate;
};

============================GLIBC ARM===============================
struct user_regs
{
  unsigned long int uregs[18];
};

#endif

#if defined(__aarch64__)//armv7a compatible with armv8a architecture
#define pt_regs   user_pt_regs
#define uregs     regs
#define ARM_pc    pc
#define ARM_sp    sp
#define ARM_cpsr  pstate
#define ARM_r0    regs[0]
#define ARM_lr    regs[30]
#define  CPSR_T_MASK  (1u << 5)
#endif


void ptrace_attach(pid_t target);
void ptrace_detach(pid_t target);
void ptrace_continue(pid_t target);
void ptrace_getregs(pid_t target, struct user_regs_struct* arm_regs);
void ptrace_setregs(pid_t target, struct user_regs_struct* arm_regs);
int ptrace_readdata(pid_t target_pid,unsigned char* src,unsigned char*buf,size_t size);
int ptrace_writedata(pid_t target_pid,unsigned char* dest,unsigned char* data,size_t size);
unsigned long int ptrace_retval(struct user_regs_struct * regs);
int ptrace_call_wrapper(pid_t target_pid, const char * func_name, unsigned long int func_addr, unsigned long int* parameters, int param_num, struct user_regs_struct * regs);

#endif
