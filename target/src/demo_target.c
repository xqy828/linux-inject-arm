#include <stdio.h>
#include <unistd.h>

typedef void (*custom_func_t)(void);

void original_func(void) 
{
    printf("Original function called\n");
}

custom_func_t custom_func_ptr = original_func;

int main()
{
    while (1)
    {
        custom_func_ptr();
        sleep(3);
    }
    return 0;
}

