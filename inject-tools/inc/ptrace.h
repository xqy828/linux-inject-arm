
#ifndef PTRACE_H
#define PTRACE_H

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

void ptrace_attach(pid_t target);
void ptrace_detach(pid_t target);
void ptrace_continue(pid_t target);
void ptrace_getregs(pid_t target, struct pt_regs* arm_regs);
void ptrace_setregs(pid_t target, struct pt_regs* arm_regs);
int ptrace_readdata(pid_t target_pid,unsigned char* src,unsigned char*buf,size_t size);
int ptrace_writedata(pid_t target_pid,unsigned char* dest,unsigned char* data,size_t size);
long ptrace_retval(struct pt_regs * regs);
void ptrace_call_wrapper(pid_t target_pid, const char * func_name, unsigned long int func_addr, unsigned long int* parameters, int param_num, struct pt_regs * regs);

#endif
