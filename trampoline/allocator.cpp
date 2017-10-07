#include "allocator.h"
#include <sys/mman.h>

allocator::allocator() {
    allocated = mmap(0, PAGE_SIZE * NUM_PAGES,
                     PROT_EXEC | PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    head = (void**) allocated;

    for (size_t i = 0; i < PAGE_SIZE * NUM_PAGES; i += TRAMPOLINE_SIZE)
    {
        auto node = (char*)allocated + i;
        *(void**)node = 0;
         if (i != 0)
         {
             *(void**)(node - TRAMPOLINE_SIZE) = node;
         }
    }
}

allocator &allocator::get_instance() {
    static allocator static_instance = allocator();
    return static_instance;
}

void *allocator::malloc() {
    void* pt = allocator::head;
    head = *(void**)head;
    return pt;
}

void allocator::free(void *ptr) {
    *(void**) ptr = head;
    head = (void**) ptr;
}

allocator::~allocator() {
    munmap(allocated, PAGE_SIZE * NUM_PAGES);
}