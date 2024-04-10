#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

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
pthread_mutex_t global_malloc_lock;

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
    size_t total_size;
    void* block;
    header_t* header;

    // 如果请求的内存大小为零，按照C标准，应当返回非空指针，此处简化为直接返回NULL
    if (!size) return NULL;

    // 获取全局互斥锁，用于同步多线程环境下的内存分配操作
    pthread_mutex_lock(&global_malloc_lock);

    // 尝试从现有的内存块链表中找到一个足够大且未使用的内存块
    header = get_free_block(size);
    
    // 如果找到了一个合适的内存块
    if (header)
    {
        // 将此内存块标记为已使用
        header->s.is_free = 0;
        
        // 释放全局互斥锁，允许其他线程继续进行内存操作
        pthread_mutex_unlock(&global_malloc_lock);

        // 返回用户可用的内存地址，即跳过内存块头部的地址
        return (void*)(header + 1); // 此处的header + 1是指向用户内存区域的指针
    }

    // 如果现有链表中没有合适大小的空闲块，需要分配新的内存
    total_size = sizeof(header_t) + size; // 计算新分配内存块的总大小，包括头部信息
    
    // 调用sbrk系统调用分配新内存
    block = sbrk(total_size);
    
    // 如果sbrk调用失败，无法分配内存，返回NULL
    if (block == (void*)-1)
    {        
        // 释放互斥锁并返回NULL，表示内存分配失败
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    // 初始化新分配内存块的头部信息
    header = block;
    header->s.size = size;
    header->s.is_free = 0; // 新分配的内存块自然是未被释放的
    header->s.next = NULL; // 新分配的内存块初始状态下没有下一个块

    // 更新链表状态
    if (!head) head = header; // 如果链表为空，则新分配的内存块成为头节点
    if (!tail) tail->s.next = header; // 如果链表已有节点，把新分配的内存块添加到链表尾部
	tail = header;

    // 释放全局互斥锁，允许其他线程继续进行内存操作
    pthread_mutex_unlock(&global_malloc_lock);

    // 返回用户可用的内存地址，跳过新分配内存块的头部信息
    return (void*)(header + 1);
}
void free(void *block)
{
    header_t *header, *tmp;
    void *programbreak;

    // 检查传入的内存块指针是否为空，若为空则直接返回
    if (!block)
        return;

    // 加锁，确保在多线程环境下，内存释放操作是线程安全的
    pthread_mutex_lock(&global_malloc_lock);

    // 计算指向内存块头部的指针，因为用户传入的block是指向用户内存区域的指针
    header = (header_t*)block - 1;

    // 获取当前进程的程序断点（brk），即堆的末尾
    programbreak = sbrk(0);

    // 判断传入的内存块是否位于堆的末尾
    if ((char*)block + header->s.size == programbreak) {

        // 如果是堆末尾的最后一个内存块
        if (head == tail) {
            // 清空头尾指针，表示堆已清空
            head = tail = NULL;
        } else {
            // 如果不是唯一的内存块，遍历链表找到尾节点的前一个节点
            tmp = head;
            while (tmp) {
                if (tmp->s.next == tail) {
                    // 将尾节点前一个节点的next指针设为NULL，更新尾节点
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }

        // 使用sbrk减小程序断点，释放堆末尾的内存块给操作系统
        sbrk(0 - sizeof(header_t) - header->s.size);

        // 解锁并返回
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }

    // 如果不是堆末尾的内存块，则仅将其标记为未使用（free）
    header->s.is_free = 1;

    // 解锁，完成内存释放操作
    pthread_mutex_unlock(&global_malloc_lock);
}