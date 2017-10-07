#include <iostream>
#include <ctime>
#include <cstdint>
#include <stdio.h>
#include <emmintrin.h>

// g++ <NAME>.cpp -o <NAME>.out - generate .s and executor file
// ./<NAME>.out - execute 

//g++ -std=c++11 word_counter.cpp -o word_counter.out

using namespace std;

bool is_space_now;

int easily_calc_count_of_words(const char *str, size_t sz) {
	int ans = 0;
	for (size_t i = 0; i < sz; i++) {
		if (*(str + i) == ' ') {
			is_space_now = true;
		} else if (is_space_now) {
			is_space_now = false;
			ans++;
		}
	}
	return ans;
}

int easily_calc_count_of_words(string str) {
	is_space_now = false;
    return easily_calc_count_of_words(str.c_str(), str.size()) + ((str.size() == 0 || str[0] == ' ') ? 0 : 1); 
}

int calc_count_of_words(const char *str, size_t g_sz) {
    size_t sz = g_sz;
	size_t last;

	__m128i spaces = _mm_set_epi8(32, 32, 32, 32,
	                              32, 32, 32, 32,    
                                  32, 32, 32, 32,
                                  32, 32, 32, 32);
	
    __m128i zero = _mm_set_epi32(0, 0, 0, 0);
	
	is_space_now = false;

	if (sz <= 64) {
		return easily_calc_count_of_words(str);
	}

	is_space_now = false;
	
	size_t ans = (sz == 0 || str[0] == ' ') ? 0 : 1;
	
	//cut seq symbols till address which % 16 = 0
	size_t offset = (size_t)str % 16;
    
    if (offset != 0) {
        offset = 16 - offset;
        ans += easily_calc_count_of_words(str, offset);
        sz -= offset;
        str += offset;
    }

    last = (sz - ((sz - offset) % 16) - 16);

    __m128i current_cmp, next_cmp, tmp;
    __m128i repo = _mm_set_epi32(0, 0, 0, 0);

    //pred-cond: have untreated elements more than 32
    __asm__(
    "movdqa   (%2), %1\n"
    "pcmpeqb  %1,   %0\n"
    :"=x"(next_cmp), "=x"(tmp)
    :"r"(str + offset), "0"(spaces)
    );
    
    /*
        HOW IT WORKS:

        str  :- one two   three| four    five  s|ix  
		mask :-1000100011100000|1000011110000110|0011
		shl  :-0001000111000001|0000111100001100|0110
		neg  :-1110111000111110|1111000011110011|1000
		and  :-1000100000100000|1000000010000010|0000
    */

    for (size_t i = offset; i < last; i += 16) {
        
        uint32_t mask;

 		current_cmp = next_cmp;
 		int new_index= 16 + i;
 		const char* new_str = str + new_index; 

        __m128i t0, t1, t2, t3;
        __asm__("movdqa   (%7), %3\n"     // %3 = (%7)
                "pcmpeqb   %3, %0\n"      // find spaces in %3
                "movdqa    %0, %6\n"      // %6 = %0
                "palignr   $1, %4, %6\n"  // (%4 + %6 >> 1) only 16 bytes
                "pandn      %4, %6\n"     // %6 = %4 AND %6 
                "psubsb     %6, %5\n"     // change "false" 1 to valid 1
                "paddusb    %5, %1\n"     // summarize 
                "pmovmskb   %1, %2\n"     // check on overflow
                :"=x"(next_cmp), "=x"(repo), "=r"(mask), "=x"(t0), "=x"(t1), "=x"(t2), "=x"(t3)
                :"r"(str + i + 16), "0"(spaces), "4"(current_cmp), "5"(zero), "1"(repo));  


        //builtin_popcount

        if (mask != 0 || new_index >= last) {
            __m128i tmp1, tmp2; 
            uint32_t lt, rt;
            __asm__("psadbw  %3, %0\n"
                    "movd    %0, %2\n"
                    "movhlps %0, %0\n"
                    "movd    %0, %1\n"
                    :"=x" (tmp1), "=r"(lt), "=r"(rt), "=x"(tmp2)
                    :"0"(zero), "3"(repo));
            ans += lt + rt;
            repo = zero;
        }        
    }

    offset = last;

    if(*(str + offset - 1) == ' ' && *(str + offset) != ' ') {
        ans--;
    }

    is_space_now = *(str + offset - 1) == ' ';
    for(size_t i = offset; i < sz; i++) {
        if (*(str + i) != ' ' && is_space_now) {
        	ans++;
        }
        is_space_now = *(str + i) == ' ';
    }

    return ans;
}

bool testing(int numbers_of_tests, int length_of_text) {
    for (int test = 0; test < numbers_of_tests; test++) {
		std::string s = "";
        
        for (size_t i = 0; i < length_of_text; i++) {
			if (rand() % 3 == 0) {
				s.append(" ");
			} else if (rand() % 3 == 1) {
				s.append("y");
			} else {
				s.append("o");
			}
		}

		int ans1 = calc_count_of_words(s.c_str(), s.size());
		int ans2 = easily_calc_count_of_words(s);
			
		if (ans1 != ans2) {
            cout << "Wrong!" << endl;
            cout << "Naive: " << ans2 << endl;
            cout << "Fast: " << ans1 << endl;
            cout << "Test: " << s << "." << endl;
			return false;
		} 
    }

    return true;
} 

int main() {
    srand(time(0));
    if (testing(20, 453)) {
    	cout << "Congratualtion!" << endl;
    }
    return 0;	
}