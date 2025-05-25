#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*original_func_t)(void);

static original_func_t original_func = NULL;
static original_func_t* target_func_ptr = NULL;

// new function
void new_func(void)
{
    printf("Hooked function called\n");
}

unsigned long get_global_variable_address(const char *executable_path, const char *variable_name)
{
    char command[256];
    FILE *pipe;
    char line[256];
    unsigned long address = 0;
    int field = 0;

    snprintf(command, sizeof(command), "readelf -s %s | grep '%s'", executable_path, variable_name);

    pipe = popen(command, "r");
    if (!pipe)
    {
        perror("popen");
        return 0;
    }

    while (fgets(line, sizeof(line), pipe) != NULL)
    {
        if (strstr(line, variable_name) != NULL)
        {
/*
pi@raspberrypi:~/workspace/ptrace $ readelf -s target | grep custom_func_ptr
98: 0000000000420038     8 OBJECT  GLOBAL DEFAULT   23 custom_func_ptr
pi@raspberrypi:~/workspace/ptrace $
*/
            char *token = strtok(line, " \t");
            while (token != NULL)
            {
                if (field == 1)
                {
                    address = strtoul(token, NULL, 16);
                    break;
                }
                token = strtok(NULL, " \t");
                field++;
            }
            break;
        }
    }
    pclose(pipe);
    return address;
}


__attribute__((constructor)) void init()
{
    unsigned long int address = 0;

    address = get_global_variable_address("/home/target","custom_func_ptr");
    if(address != 0)
    {
        printf("find target func:%s address:0x%lx\r\n","custom_func_ptr",address);
    }
    else
    {
        printf("Error: Failed to find 'custom_func_ptr'\r\n");
    }
    target_func_ptr = (original_func_t*)address;
    original_func = *target_func_ptr;
/*
    size_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t addr = (uintptr_t)target_func_ptr;
    uintptr_t aligned_addr = addr & ~(page_size - 1);
    if (mprotect((void*)aligned_addr, page_size, PROT_READ | PROT_WRITE) == -1)
    {
        perror("mprotect");
        return;
    }
*/
    *target_func_ptr = new_func;
/*
    mprotect((void*)aligned_addr, page_size, PROT_READ);
*/
    printf("Hook installed: custom_func_ptr redirected new func address:0x%lx\n",(unsigned long int)new_func);
}

__attribute__((destructor)) void cleanup()
{
    if (target_func_ptr && original_func)
    {
        *target_func_ptr = original_func;
        printf("Hook removed: original function restored\n");
    }
}

int hook_entry(char * a){
    printf("Hook success, pid = %d\n", getpid());
    printf("Hello %s\n", a);
    return 0;
}





