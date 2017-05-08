#include <stdio.h>
#include <emmintrin.h>
#include <iostream>
#include <string>

// g++ <NAME>.cpp -o <NAME>.out - generate .s and executor file
// ./<NAME>.out - execute 

void memcpy(void * dst, void const * src, size_t cnt) { 
    
    size_t offset = 0;
    size_t size_src = cnt;

    while (((size_t) dst + offset) % 16 != 0 && (offset < cnt)) {
        *((char*)(dst) + offset) = *((char*)(src) + offset);
    	offset++; 
     	cnt--;
    }

    std::cout << "After alignment (begin):\n";
    std::cout << "src: ";
    std::cout << (char*)src + offset << std::endl;
    std::cout << "dst: ";
    std::cout << (char*)dst + offset << std::endl;
    std::cout << "Left count: ";
    std::cout << cnt << std::endl;

    void * new_dst = (char*) dst + offset;
    void * new_src = (char*) src + offset;

    __m128i tmp;         // int128
    __asm__ volatile (   // open asm-insert without optimization
    "work:\n"            // set label "work"
    "cmpq  $16, %2\n"    // %2(the same as cnt) - 16 < 0
    "jb ex\n"            // if (%2 - 16 < 0) then go to label "ex"
    "movdqu  (%1), %3\n" // %3(tsa tmp) := (%1)(tsa *new_src); can be no aligned; MEM -> REG
    "movntdq %3, (%0)\n" // (%0)(tsa *new_dst) = %3; without caching, must be aligned; REG -> MEM
    "add     $16,  %0\n" // %0 = %0 + 16 bytes 
    "add     $16,  %1\n" // %1 = %1 + 16 bytes
    "sub     $16,  %2\n" // %2 = %2 - 16 bytes
    "jmp     work\n"     // jump to label "work"
    "ex:\n"              // set label ex (it is empty, special for exit)
    :"=r"(new_dst), "=r"(new_src), "=r"(cnt), "=x"(tmp)   // OUTPUT: i want in order to ultimate result will be in:
						          // new_dist = %0, new_src = %1 ...
    :"0"(new_dst), "1"(new_src), "2"(cnt)                 // INPUT: give me registers! i don't know about their names
				                          // but i want define their as(with default values):
				                          // %0 = new_dist, %1 = new_src ... 
    :"memory" //rewrite memory
    );

    std::cout << "====Vectorization====" << std::endl;
    std::cout << "Left after vectorization: ";
    std::cout << (char*) new_src << std::endl;
    std::cout << "Left count: ";
    std::cout << cnt << std::endl;
    std::cout << "dst: ";
    std::cout << (char*) dst << std::endl;

    

    for (size_t i = 0; i < cnt; i++) {
    	*((char*)(new_dst) + i) = *((char*)(new_src) + i);
    }

 	std::cout << "After alignment (end):\n";
 	std::cout << "dst: ";
    std::cout << (char*) dst << std::endl;
    _mm_sfence(); 
}



int main() {
    std::string a = "1111111111222222222233333333334444444444555555555566666666667777777777";
    char *buffer = new char[a.size()];
    memcpy(buffer, a.c_str(), a.size());
    std::cout << "====RESULT====" << std::endl;
    std::cout << "dst: ";
    std::cout << buffer << std::endl;
    return 0;
}
