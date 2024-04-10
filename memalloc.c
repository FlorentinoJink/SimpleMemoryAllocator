#include <unistd.h>

typedef char ALIGN[16];

union header {
	struct {
		size_t size;
		unsigned is_free;
		union header *next;
	} s;
	ALIGN stub;
};
typedef union header header_t;

header_t* head = NULL, *tail = NULL;

header_t* get_free_block(size_t size)
{
    header_t* curr = head;
    while (curr)
    {
        if (curr->s.is_free && curr->s.size >= size)
        {
            return curr;
        }
        curr = curr->s.next;
    }
    return NULL;
}


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