#ifndef IMM_H
#define IMM_H
 
struct imm {
    union {
        void* addr;
        int32_t i;
    };
    size_t size;

    imm(void *addr, size_t size):size(size), addr(addr){}
    imm(int32_t i, size_t size):size(size), i(i){}
};

#endif