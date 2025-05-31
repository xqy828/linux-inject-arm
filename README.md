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
OS:   
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
测试log:   
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
