# linux-inject-arm

## 介绍

linux inject for arm
参考项目：     
https://github.com/gaffe23/linux-inject/
https://github.com/itwenhao123456/-Android-Linux-ARM-Hook

## 使用说明
./Inject.out -p 44361 ./demo_library.so   

## 测试
### Raspberrypi 4B   
#### OS:   
```js
pi@raspberrypi:~ $ uname -a
Linux raspberrypi 6.6.74+rpt-rpi-v8 #1 SMP PREEMPT Debian 1:6.6.74-1+rpt1 (2025-01-27) aarch64 GNU/Linux
pi@raspberrypi:~ $ ldd --version
ldd (Debian GLIBC 2.36-9+rpt2+deb12u8) 2.36
Copyright (C) 2022 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
Written by Roland McGrath and Ulrich Drepper.
pi@raspberrypi:~ $
```
#### 测试log:   
```js
pi@raspberrypi:~ $ ./Inject.out -p 44361 ./demo_library.so
[-[+]-]:main-(00513)]Compiled with glibc: 2.38
[-[+]-]:main-(00514)]Running on glibc: 2.36
[-[+]-]:main-(00537)]Target process pid 44361
[-[+]-]:main-(00545)]Inject library /home/pi/demo_library.so
[-[+]-]:injectProcess-(00287)]Injecting process: 44361
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0x7fa4320000], target libc.so.6[0x7fb7910000],funcAddrOffset 932320 target LibcFuncAddr[0x7fb79f39e0]
[-[+]-]:injectProcess-(00290)]Target process mmap address: 0x7fb79f39e0
[-[+]-]:injectProcess-(00301)]Running GLIBC version: 2.36.0
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0x7fa4320000], target libc.so.6[0x7fb7910000],funcAddrOffset 503360 target LibcFuncAddr[0x7fb798ae40]
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0x7fa4320000], target libc.so.6[0x7fb7910000],funcAddrOffset 503600 target LibcFuncAddr[0x7fb798af30]
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0x7fa4320000], target libc.so.6[0x7fb7910000],funcAddrOffset 501120 target LibcFuncAddr[0x7fb798a580]
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0x7fa4320000], target libc.so.6[0x7fb7910000],funcAddrOffset 501184 target LibcFuncAddr[0x7fb798a5c0]
[-[+]-]:injectProcess-(00387)]Get imports: dlopen: 0x7fb798ae40, dlsym: 0x7fb798af30, dlclose: 0x7fb798a580, dlerror: 0x7fb798a5c0
[-[+]-]:injectProcess-(00388)]library path = /home/pi/demo_library.so
[-[+]-]:ptrace_call_wrapper-(00212)]Calling mmap in target process.
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from mmap, return value=7fb790c000, pc=0
[-[+]-]:injectProcess-(00407)]Target process after mmap map_base = 0x7fb790c000
[-[+]-]:ptrace_call_wrapper-(00212)]Calling __libc_dlopen_mode/dlopen in target process.
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from __libc_dlopen_mode/dlopen, return value=126ba6e0, pc=0
[-[+]-]:injectProcess-(00435)]"/home/pi/demo_library.so" successfully injected
[-[+]-]:ptrace_call_wrapper-(00212)]Calling __libc_dlsym/dlsym in target process.
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from __libc_dlsym/dlsym, return value=7fb78e0af0, pc=0
[-[+]-]:injectProcess-(00468)]hook_entry_addr = 7fb78e0af0
[-[+]-]:ptrace_call_wrapper-(00212)]Calling hook_entry in target process.
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from hook_entry, return value=0, pc=0
[-[+]-]:injectProcess-(00479)]Press enter to dlclose and detach

[-[+]-]:ptrace_call_wrapper-(00212)]Calling __libc_dlclose/dlclose in target process.
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from __libc_dlclose/dlclose, return value=0, pc=0
pi@raspberrypi:~ $

Original function called
Hooked function auto start
Hook success, pid = 44361
Hello hello hook
Original function called
Original function called

```
### NXP i.MX93(9352)
#### OS: 
```js
root@myd-lmx9x:~# uname -a
Linux myd-lmx9x 6.1.55+ga5c552ce6210 #1 SMP PREEMPT Thu Apr 11 02:15:58 UTC 2024 aarch64 GNU/Linux
root@myd-lmx9x:~#
root@myd-lmx9x:~#
root@myd-lmx9x:~# /lib/libc.so.6
GNU C Library (GNU libc) stable release version 2.37.
Copyright (C) 2023 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
Compiled by GNU CC version 12.3.0.
libc ABIs: UNIQUE ABSOLUTE
Minimum supported kernel: 3.14.0
For bug reporting instructions, please see:
<https://www.gnu.org/software/libc/bugs.html>.
root@myd-lmx9x:~#
root@myd-lmx9x:~#
root@myd-lmx9x:~#

```
#### 测试log
```js
root@myd-lmx9x:~# 
root@myd-lmx9x:~# ./demo_process.out &
[1] 707
Original function called
root@myd-lmx9x:~# Original function called
Original function called

root@myd-lmx9x:~# Original function called
./Inject.out -p 707 ./demo_library.so
[-[+]-]:main-(00513)]Compiled with glibc: 2.38
[-[+]-]:main-(00514)]Running on glibc: 2.37
[-[+]-]:main-(00537)]Target process pid 707
[-[+]-]:main-(00545)]Inject library /home/root/demo_library.so
[-[+]-]:injectProcess-(00287)]Injecting process: 707
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0xffff943e0000], target libc.so.6[0xffffa9a40000],funcAddrOffset 923824 target LibcFuncAddr[0xffffa9b218b0]
[-[+]-]:injectProcess-(00290)]Target process mmap address: 0xffffa9b218b0
[-[+]-]:injectProcess-(00301)]Running GLIBC version: 2.37.0
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0xffff943e0000], target libc.so.6[0xffffa9a40000],funcAddrOffset 498560 target LibcFuncAddr[0xffffa9ab9b80]
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0xffff943e0000], target libc.so.6[0xffffa9a40000],funcAddrOffset 498756 target LibcFuncAddr[0xffffa9ab9c44]
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0xffff943e0000], target libc.so.6[0xffffa9a40000],funcAddrOffset 496464 target LibcFuncAddr[0xffffa9ab9350]
[-[+]-]:getTargetProcessLibcFuncAddr-(00245)]local libc.so.6[0xffff943e0000], target libc.so.6[0xffffa9a40000],funcAddrOffset 496544 target LibcFuncAddr[0xffffa9ab93a0]
[-[+]-]:injectProcess-(00387)]Get imports: dlopen: 0xffffa9ab9b80, dlsym: 0xffffa9ab9c44, dlclose: 0xffffa9ab9350, dlerror: 0xffffa9ab93a0
[-[+]-]:injectProcess-(00388)]library path = /home/root/demo_library.so
[-[+]-]:ptrace_call_wrapper-(00212)]Calling mmap in target process.
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from mmap, return value=ffffa9c2c000, pc=0 
[-[+]-]:injectProcess-(00407)]Target process after mmap map_base = 0xffffa9c2c000
[-[+]-]:ptrace_call_wrapper-(00212)]Calling __libc_dlopen_mode/dlopen in target process.
Hooked function auto start
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from __libc_dlopen_mode/dlopen, return value=81a02e0, pc=0 
[-[+]-]:injectProcess-(00435)]"/home/root/demo_library.so" successfully injected
[-[+]-]:ptrace_call_wrapper-(00212)]Calling __libc_dlsym/dlsym in target process.
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from __libc_dlsym/dlsym, return value=ffffa9a10af0, pc=0 
[-[+]-]:injectProcess-(00468)]hook_entry_addr = ffffa9a10af0
[-[+]-]:ptrace_call_wrapper-(00212)]Calling hook_entry in target process.
Hook success, pid = 707
Hello hello hook
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from hook_entry, return value=0, pc=0 
[-[+]-]:injectProcess-(00479)]Press enter to dlclose and detach

[-[+]-]:ptrace_call_wrapper-(00212)]Calling __libc_dlclose/dlclose in target process.
[-[+]-]:ptrace_call_wrapper-(00218)]Target process returned from __libc_dlclose/dlclose, return value=0, pc=0 
root@myd-lmx9x:~# 
root@myd-lmx9x:~# Original function called
Original function called
Original function called
Original function called

```






