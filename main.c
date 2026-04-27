#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Δήλωση της εξωτερικής συνάρτησης Assembly */
extern int access_hash(char* pin);

/* * Η "πηγή της αλήθειας" σε C. 
 * Ακολουθεί ακριβώς τους κανόνες της εκφώνησης για να ελέγξουμε την Assembly.
 */
int calculate_expected_hash(const char* pin) {
    int length = strlen(pin);
    int hash = length; // 1. Αρχική τιμή το μήκος
    
    // Ο πίνακας αναφοράς (Look-up Table) για τους αριθμούς 0-9
    int lut[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    
    for (int i = 0; i < length; i++) {
        char c = pin[i];
        
        if (c >= '0' && c <= '9') {
            // 2. Είναι αριθμός: Προσθήκη από το LUT
            hash += lut[c - '0'];
        } 
        else if (c >= 'A' && c <= 'Z') {
            // 3. Είναι κεφαλαίο: ASCII * 2
            hash += (c * 2);
        } 
        else if (c >= 'a' && c <= 'z') {
            // 4. Είναι πεζό: Μάσκα AND 0xDF για να γίνει κεφαλαίο και προσθήκη
            char upper_c = c & 0xDF; 
            hash += upper_c;
        }
        // Κάθε άλλος χαρακτήρας (π.χ. @, #, κενό) αγνοείται αυτόματα
    }
    
    return hash;
}

/* * Βοηθητική συνάρτηση για παραγωγή τυχαίων αλφαριθμητικών 
 */
void generate_random_string(char *str, int length) {
    // Περιλαμβάνει μικρά, κεφαλαία, αριθμούς και σύμβολα (για να ελέγξουμε αν τα αγνοεί σωστά)
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$*";
    int charset_size = sizeof(charset) - 1;
    
    for (int i = 0; i < length; i++) {
        str[i] = charset[rand() % charset_size];
    }
    str[length] = '\0'; // Απαραίτητος χαρακτήρας τερματισμού
}

int main() {
    // Αρχικοποίηση γεννήτριας τυχαίων αριθμών
    srand(time(NULL));
    
    int num_tests = 100;
    int errors = 0;
    
    printf("--- Ξεκινάει το Testbench (%d δοκιμές) ---\n\n", num_tests);
    
    for (int i = 1; i <= num_tests; i++) {
        char pin[50];
        
        // Επιλογή τυχαίου μήκους για το string (από 1 έως 20 χαρακτήρες)
        int random_length = (rand() % 20) + 1; 
        
        // Παραγωγή τυχαίου PIN
        generate_random_string(pin, random_length);
        
        // Υπολογισμός και από τις δύο πλευρές
        int expected_hash = calculate_expected_hash(pin);
        int asm_hash = access_hash(pin);
        
        // Σύγκριση και αναφορά σφάλματος
        if (expected_hash != asm_hash) {
            printf("[ΣΦΑΛΜΑ] Test %d απέτυχε!\n", i);
            printf("  PIN      : '%s'\n", pin);
            printf("  Αναμενόταν: %d (C)\n", expected_hash);
            printf("  Παρήχθη   : %d (Assembly)\n", asm_hash);
            printf("------------------------------------------\n");
            errors++;
        }
    }
    
    // Τελικό Report
    if (errors == 0) {
        printf("\n[ ΕΠΙΤΥΧΙΑ ] Όλα τα %d test πέρασαν χωρίς κανένα σφάλμα!\n", num_tests);
        printf("Η Assembly συνάρτηση είναι αλάνθαστη.\n");
    } else {
        printf("\n[ ΑΠΟΤΥΧΙΑ ] Βρέθηκαν %d σφάλματα σε %d δοκιμές.\n", errors, num_tests);
    }
    
    return 0;
}