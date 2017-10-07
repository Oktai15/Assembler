#ifndef TRAMPOLINE_H
#define TRAMPOLINE_H

#include <string>
#include <iostream>
#include "allocator.h"
#include "imm.h"
#include "numerator_arguments.h"

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

    const char* commands_for_shifting[6] = {
        //REGISTERS: RDI - RSI - RDX - RCX - R8 - R9
        "\x48\x89\xfe", // mov rsi rdi 
        "\x48\x89\xf2", // mov rdx rsi 
        "\x48\x89\xd1", // mov rcx rdx 
        "\x49\x89\xc8", // mov r8 rcx  
        "\x4d\x89\xc1", // mov r9 r8   
        "\x41\x51"      // push r9   
    };

public:
    template <typename F>
    trampoline(F func) : func_obj(new F((func))), deleter(deleter_func<F>) {
        
        start_of_block = allocator::get_instance().malloc();
        current_pointer_of_block = (char*)start_of_block;
        
        if (Numerator<Args ...>::INTEGER < 6) {
            //pre: only registers
            //post: shift arguments
            for (int i = Numerator<Args ...>::INTEGER - 1; i >= 0 ; i--) {
                push_bytes(commands_for_shifting[i]);
            }
            //post: add func_obj to RDI and then add &do_call to RAX so that we can start
            push_bytes("\x48\xbf"); push_bytes(imm(func_obj, 8));                   // mov  rdi func_obj
            push_bytes("\x48\xb8"); push_bytes(imm((void *)&do_call<F>, 8));        // mov  rax &do_call
            push_bytes("\xff\xe0");                                                 // jmp  rax
        } else {    
            //pre: arguments is located on registers and stack
            //post: save address for returning, which is located on top of stack ([rsp]), in r11
            push_bytes("\x4c\x8b\x1c\x24");                            // mov  r11 [rsp]

            //post: shift arguments and push last to stack
            for (int i = 5 ; i >= 0; i--) {
                push_bytes(commands_for_shifting[i]);
            }

            //Stack now:
            //arg
            //....
            //arg
            //arg
            //ret
            //element, which we pushed from register (arg too)

            //size of arguments on stack
            int stack_size = 8 * (Numerator<Args ...>::INTEGER - 5 + std::max(Numerator<Args ...>::SSE - 8, 0));

            //post: find arguments borders
            push_bytes("\x48\x89\xe0");                                // mov  rax rsp
            push_bytes("\x48\x05"); push_bytes(imm(stack_size, 4));    // add  rax stack_size
            push_bytes("\x48\x81\xc4"); push_bytes(imm(8, 4));         // add  rsp 8

            //Stack now:
            //arg <- rax
            //....
            //arg
            //arg
            //ret <- rsp
            //element, which we pushed from register (arg too)

            char* lbl = current_pointer_of_block; //start of cycle

            //post: shift all arguments in stack
            push_bytes("\x48\x39\xe0");                                 // cmp  rax rsp
            push_bytes("\x74\x12");                                     // je   18 bytes -> next section
            push_bytes("\x48\x81\xc4"); push_bytes(imm(8, 4));          // add  rsp 8
            push_bytes("\x48\x8b\x3c\x24");                             // mov  rdi [rsp]
            push_bytes("\x48\x89\x7c\x24\xf8");                         // mov  [rsp - 8] rdi
            push_bytes("\xeb");                                         // jmp  lbl
            
            *current_pointer_of_block = lbl - current_pointer_of_block++ - 1; //end of iteration by cycle

            //Stack now:
            //ret <- rsp <- rax
            //arg 
            //....
            //arg
            //arg
            //element, which we pushed from register (arg too)

            //pre: all arguments are shifted
            push_bytes("\x4c\x89\x1c\x24");                                 // mov  [rsp] r11 
            push_bytes("\x48\x81\xec"); push_bytes(imm(stack_size, 4));     // sub  rsp stack_size
            push_bytes("\x48\xbf"); push_bytes(imm(func_obj, 8));           // mov  rdi func_obj
            push_bytes("\x48\xb8"); push_bytes(imm((void*)&do_call<F>, 8)); // mov  rax &do_call
            push_bytes("\xff\xd0");                                         // call rax             

            //Stack now:
            //ret (from r11)
            //arg 
            //....
            //arg
            //arg
            //element, which we pushed from register (arg too) <- rsp

            //pre: function is done
            //post: restore previous state of stack (r11) due to calling convention
            push_bytes("\x41\x59");                                                 // pop   r9
            push_bytes("\x4c\x8b\x9c\x24"); push_bytes(imm(stack_size - 8, 4));     // mov   r11 [rsp + stack_size - 8]
            push_bytes("\x4c\x89\x1c\x24");                                         // mov   [rsp] r11
            push_bytes("\xc3");                                                     // ret           
        }
    }

    //execute passed function by using it    
    template <typename F>
    static T do_call(void* obj, Args ...args) {
        return (*(F*)obj)(args...);
    }

    //returns point to function which passes arguments (Args..,) and returns T
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