#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// --- ????se?? t?? e??te????? s??a?t?se?? Assembly ---
extern int access_hash(char* pin);
extern int clearence(int hash);
extern int lucas_sequence(int n); // ? 3? S????t?s?

// ==========================================
// C-EQUIVALENTS (? "????e?a" ??a t?? ??e???)
// ==========================================

// 1. ?p?????sµ?? Hash
int calculate_expected_hash(const char* pin) {
    int length = strlen(pin);
    int hash = length; 
    int lut[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    
    for (int i = 0; i < length; i++) {
        char c = pin[i];
        if (c >= '0' && c <= '9') { hash += lut[c - '0']; } 
        else if (c >= 'A' && c <= 'Z') { hash += (c * 2); } 
        else if (c >= 'a' && c <= 'z') { hash += (c & 0xDF); }
    }
    return hash;
}

// 2. ?p?????sµ?? Clearance Level (?ss?? % 6)
int calculate_expected_clearence(int hash) {
    int count = 0;
    unsigned int n = (unsigned int)hash; 
    while (n > 0) {
        count += (n & 1); 
        n >>= 1;          
    }
    return count % 6;
}

// 3. ?p?????sµ?? Lucas Sequence (??ad??µ???)
int calculate_expected_lucas(int n) {
    if (n == 0) return 2;
    if (n == 1) return 1;
    return calculate_expected_lucas(n - 1) + calculate_expected_lucas(n - 2);
}

// ==========================================
// ???T????? S??????S? ????GOG?S PIN
// ==========================================
void generate_random_string(char *str, int length) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$*";
    int charset_size = sizeof(charset) - 1;
    for (int i = 0; i < length; i++) {
        str[i] = charset[rand() % charset_size];
    }
    str[length] = '\0'; 
}

// ==========================================
// ????? ???G????? (TESTBENCH)
// ==========================================
int main() {
    srand(time(NULL));
    int num_tests = 100;
    int errors = 0;
    
    printf("--- ?e????e? t? Testbench (%d tests) ---\n\n", num_tests);
    
    for (int i = 1; i <= num_tests; i++) {
        char pin[50];
        int random_length = (rand() % 20) + 1; 
        generate_random_string(pin, random_length);
        
        // --- ??????S? C ---
        int expected_hash = calculate_expected_hash(pin);
        int expected_clearance = calculate_expected_clearence(expected_hash);
        int expected_lucas = calculate_expected_lucas(expected_clearance);
        
        // --- ??????S? ASSEMBLY ---
        int asm_hash = access_hash(pin);
        int asm_clearance = clearence(asm_hash);
        int asm_lucas = lucas_sequence(asm_clearance);
        
        // --- S?G???S? ???????S???O? ---
        if (expected_hash != asm_hash || expected_clearance != asm_clearance || expected_lucas != asm_lucas) {
            printf("[SF????] Test %d ap?t??e!\n", i);
            printf("  PIN      : '%s'\n", pin);
            
            if (expected_hash != asm_hash) {
                printf("  HASH Expected      : %d (C) | Got: %d (ASM)\n", expected_hash, asm_hash);
            }
            if (expected_clearance != asm_clearance) {
                printf("  CLEARANCE Expected : %d (C) | Got: %d (ASM)\n", expected_clearance, asm_clearance);
            }
            if (expected_lucas != asm_lucas) {
                printf("  LUCAS Expected     : %d (C) | Got: %d (ASM)\n", expected_lucas, asm_lucas);
            }
            printf("------------------------------------------\n");
            errors++;
        }
    }
    
    // --- ?????? REPORT ---
    if (errors == 0) {
        printf("\n[ success ] ??a ta %d tests perasan!\n", num_tests);
   
    } else {
        printf("\n[ failed ]  %d \n", errors);
    }
    
    return 0;
}