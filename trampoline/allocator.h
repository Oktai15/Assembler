#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstdio>

//Allocator-Singleton
class allocator {

public:
    static allocator& get_instance();

    void *malloc();
    void free(void *ptr);

private:
    allocator();
    ~allocator();

    static const size_t NUM_PAGES = 1;
    static const size_t PAGE_SIZE = 4096;
    static const size_t TRAMPOLINE_SIZE = 256;

    void *allocated;
    void *head;

};

#endif // ALLOCATOR_H