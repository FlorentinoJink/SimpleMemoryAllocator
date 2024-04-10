#include <unistd.h>

void* malloc(size_t size) 
{
    void* block;
    block = sbrk(size);
    if (block == (void*)-1)
    {
        reutrn NULL;
    }
    return block;
}