#include <stdio.h>
#include <string.h>


int lucas_test(int n);
void test_func(char *pin, int hash_real, int clearance_real, int lucas_real, int bonuss);
extern int hash(char *a);
extern int clearance(int b);
extern int lucas(int c);
extern int checksum(char *pin);

//main
int main (){
    int hash_res, clearance_res, final_res, bonuss;
    char pin[] = "Ab45j";
    
    hash_res = hash(pin);
    printf("Hash result is: %d \n", hash_res);
    
    clearance_res = clearance(hash_res);
    printf("Clearance result is : %d\n", clearance_res); 
    
    final_res = lucas(clearance_res);
    printf("Final result is : %d \n", final_res);
	
	  bonuss = checksum(pin);
    
    // Time for testing
    test_func(pin, hash_res, clearance_res, final_res, bonuss);
    
    return 0;
}

// Lucas for the tesh
int lucas_test(int n){
    if(n == 0)
        return 2;
    else if(n == 1)
        return 1;
    else 
       return lucas_test(n-1) + lucas_test(n-2);    
}
//Actual test function
void test_func(char *pin, int hash_real, int clearance_real, int lucas_real, int bonuss){
    int expected_hash, clearance_lvl, lucas_res, ones_cnt;
    int lookup_table[10]= {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    
    expected_hash = 0 + strlen(pin);
    int i = 0;
    
    // Make hash
    while(pin[i] != '\0'){
        if((pin[i] >= 48) && (pin[i] <= 57)){
            expected_hash += lookup_table[pin[i] - 48];
        }
        else if((pin[i] >= 65) && (pin[i] <= 90)){
            expected_hash += pin[i]*2;
        }
        else if((pin[i] >= 97) && (pin[i] <= 122)){
            expected_hash += (pin[i] - 32);
        }
        else{
            expected_hash += 0;
        }
        i++;
    }
    
    if(expected_hash != hash_real){
        printf("Hash does not match real hash.\n");
        printf("Expected hash: %d, Real hash: %d\n", expected_hash, hash_real);
    } else {
        printf("HASH MATCH\n");
    }
    
    // Clearance level & Number of ones
    ones_cnt = 0;
    int temp_hash = expected_hash; // to keep the hash incase we need it later on
    
    for(i = 0; i < 32; i++){
        if((temp_hash) & 1)
            ones_cnt++;
        temp_hash = temp_hash >> 1;      
    }
    clearance_lvl = ones_cnt % 6;
    
    // Lucas number
    lucas_res = lucas_test(clearance_lvl);
   
    if(clearance_lvl != clearance_real){
        printf("Clearance level does not match real clearance level.\n");
        printf("Expected clearance level: %d, Real clearance level: %d\n", clearance_lvl, clearance_real);
    } else {
        printf("Clearance level MATCH\n");
    }
    
    if(lucas_res != lucas_real){
        printf("Lucas number does not match real lucas number.\n");
        printf("Expected lucas number: %d, Real lucas number: %d\n", lucas_res, lucas_real);
    } else {
        printf("LUCAS RESULT MATCH\n");
    }

   //Bonus check
    int checksum_result = 0;
    unsigned char current_checksum = 0;
    i = 0;
    while (pin[i] != '\0') {
       
        unsigned char salted_char = (unsigned char)(pin[i]) ^ 0xAA;
        current_checksum ^= salted_char;
        i++;
    }
    checksum_result = (int)current_checksum;
    if(checksum_result != bonuss){
        printf("Bonus check does not match real bonus.\n");
        printf("Expected bonus: %d, Real bonus: %d\n", checksum_result, bonuss);
    } else {
        printf("BONUS CHECK MATCH\n");
    }

}