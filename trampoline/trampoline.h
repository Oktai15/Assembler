#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <string>
#include <iostream>
#include "allocator.h"
#include "imm.h"

template <typename T>
struct trampoline;

// Trampoline "eats" function as return type (T) and functions arguments (Args ...).
template <typename T, typename ... Args>
struct trampoline<T (Args ...)> {
private:

    void *start_of_block;
    char *current_pointer_of_block;

    void* func_obj;
    void (*deleter)(void*);


public:
    template <typename F>
    trampoline(F func) : func_obj(new F((func))), deleter(deleter_func<F>) {
        
        start_of_block = allocator::get_instance().malloc();
        current_pointer_of_block = (char*)start_of_block;
                       
        push_bytes("\x49\xbb"); push_bytes(imm(func_obj, 8));           // mov func_obj r11
        push_bytes("\x48\xb8"); push_bytes(imm((void*)&do_call<F>, 8)); // mov &do_call rax
        push_bytes("\xff\xe0");                                         // jmp rax           
    }

    //execute passed function by using it    
    template <typename F>
    static T do_call(Args ...args) {

    	void* obj;

        __asm__ volatile (
        "movq %%r11, %0\n"        
        :"=x"(obj)
        :"0"(obj)
        : "r11"
        );

        return (*(F*)obj)(args...);
    }

    //return point to function to which passes arguments (Args..,) and returns T
    T (*get() const)(Args ... args) {
        return (T(*)(Args ... args))start_of_block;
    }

    void push_bytes(std::string const& set_of_bytes) {
        for (const char *i = set_of_bytes.c_str(); i < set_of_bytes.c_str() + set_of_bytes.size(); i++) {
            *(current_pointer_of_block++) = *i;
        }
    }

    void push_bytes(imm im) {
        *(void**)current_pointer_of_block = im.addr;
        current_pointer_of_block += im.size;
    }

    trampoline& operator=(trampoline&& other) {
        func_obj = other.fun_obj;
        start_of_block = other.start_of_block;
        deleter = other.deleter;
        other.fun_obj = nullptr;
        return *this;
    }

    trampoline(trampoline&& other) {
        func_obj = other.func_obj;
        start_of_block = other.start_of_block;
        deleter = other.deleter;
        other.func_obj = nullptr;
    }

    ~trampoline() {
        if (func_obj != nullptr) {
            deleter(func_obj);
        }
        allocator::get_instance().free(start_of_block);
    }

private:
    template <typename F>
    static void deleter_func(void* func_obj) {
        delete (F*)(func_obj);
    }
};

#endif