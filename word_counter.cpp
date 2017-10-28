#include <iostream>
#include <ctime>
#include <cstdint>
#include <stdio.h>
#include <emmintrin.h>

// g++ <NAME>.cpp -o <NAME>.out - generate .s and executor file
// ./<NAME>.out - execute 

//g++ -std=c++11 word_counter.cpp -o word_counter.out

using namespace std;

int easily_calc_count_of_words(const char *str, size_t sz) {
	bool is_space_now = false;
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
	
	//is_space_now = false;

	if (sz <= 64) {
		return easily_calc_count_of_words(str);
	}

	//is_space_now = false;
	
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


    /*
        HOW IT WORKS:

        str  :- one two   three| four    five  s|ix  
		mask :-1000100011100000|1000011110000110|0011
		shl  :-0001000111000001|0000111100001100|0110
		neg  :-1110111000111110|1111000011110011|1000
		and  :-1000100000100000|1000000010000010|0000
    
    */


    //pre: have untreated elements more than 32
    __asm__ volatile(
    "movdqa     (%0), %1\n" // %1 = %0 (next_cmp = first 16 bytes in str)
    "pcmpeqb     %2,  %1\n" // find spaces in %1 using cmp with spaces
    :"=r" (str),"=x"(next_cmp), "=x"(spaces)
    :"0"(str), "2"(spaces)
    );

    //we will use next_cmp (which was mentioned above) as current_tmp here and so on...
    for (sz; sz >= 16; sz -= 16) {
        uint32_t mask;
        __asm__ volatile(
        "add        $16,  %0\n"     // %0 = %0 + 16 (next block in str)
        "movdqa     %3,   %1\n"     // %1 = %3 (cur is pred next)
        "movdqa    (%0),  %2\n"     // %2 = (%0) (get next block in tmp)
        "pcmpeqb    %4,   %2\n"     // find spaces in %2 using cmp with spaces 
        "movdqa     %2,   %3\n"     // %3 = %2 (next = temp)
        "palignr    $1,   %1, %2\n" // (%1 + %2 >> 1) only first 16 bytes
        "pandn      %1,   %2\n"     // %2 = %2 and not %1
        "pmovmskb   %2,   %5\n"     // high-order bytes, go to mask 
        :"=r"(str), "=x"(current_cmp), "=x"(tmp), "=x"(next_cmp), "=x"(spaces), "=r"(mask)
        :"0"(str), "1"(current_cmp), "2"(tmp), "3"(next_cmp), "4"(spaces), "5"(mask)
        );


        ans += __builtin_popcount(mask); //calc h-o bytes
    }

    //is_space_now = false;
    return ans + easily_calc_count_of_words(str, sz);
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
    if (testing(100, 5348)) {
    	cout << "Congratualtion!" << endl;
    }
    return 0;	
}