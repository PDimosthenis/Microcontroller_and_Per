    .global access_hash
    .p2align 2
    .type access_hash,%function
        
        access_hash:
        .fnstart
        
        @ R4 R5 stack
        STMFD SP!, {R4, R5}
        
        MOV R1 , #0 @θα είναι ο καταχωρητής που θα μετρήσω το μήκος του string
        MOV R3 , R0 @ την διευθυνση την βαζω καπου αλλου 
        MOV R12 , #0 @εδω θα εχω το sum μου   
    branch :
            LDRB R2 , [R3] @ R2 = value_of(mem(R0)) , δηλαδη ο R2 παιρνει την τιμη της θεσης μνημης R0
            CMP R2 , #0      @ Συγκρίνει την τιμή του R2 με το 0
            BEQ end_loop    @ Branch if EQual: Αν ήταν ίσα Zero flag=1 πήγαινε στο end_loop
            ADD R1 , R1 , #1  @ R1 = R1 + 1 , αυξησε τον καταχωρητη του μηκους κατα ενα
            ADD R3 , R3 , #1 @ ο δεικτης που σκαναρει το στρινγκ +1 επειδη καθε γραμμα ειναι ενα byte αρα +1
            @τωρα τελειωσε το κομματι του αν εφτασε στο τελος του στρινγκ 

            @τσεκαρω τα εξης επειδη τα νουμερα ξεκιναν  πρωτα θα τσεκαρω πρωτα αν τα χω φτασει
            CMP R2 , #'0'
            BLO branch @Αν ειναι μικροτερο του 0 απλα πανε στον επομενο

            CMP R2 , #'9'
            BLE number_handler

            CMP R2 , #'A'
            BLO branch

            CMP R2 , # 'Z'
            BLE upper_handler

            CMP R2 , # 'a'
            BLO branch

            CMP R2 , #'z'
            BLE lower_handler

            B branch
    end_loop :
              @store thn timh
              add R0, R12 , R1 @ η τελικη τιμη ειναι οτι βγαλαν απο τα στρινγκσ συν το μηκος που αναφερεται 
              
              @ teleiwsame skoupistikame
              LDMFD SP!, {R4, R5}
              
              bx lr @ i guess epistrefw tin timh?

number_handler:
    SUB R2, R2, #'0'        @ Επειδη το asci του 0 δεν ειναι 0 του 1 το 1 κλπ κπλ  αφαιρω την σταθερη διαφορα δηλαδη την αρχικη τιμη και ετσι εχω σωστη αντιστοιχιση
    @προσοχη σωσε καπου R4 ,R5,R12 (Σημείωση: Οι R4, R5 σώθηκαν στο Prologue, ο R12 δεν το χρειάζεται!)
    ADR R4, lut_numbers     @ παιρνω το base addres του LUT 
    
    LDR R5, [R4, R2, LSL #2] @ R5 = mem[ R4 + (R2 * 4) ]
    
    ADD R12, R12, R5        @ hash = hash + R5
    
    B branch               


upper_handler :
    ADD R12, R12, R2, LSL #1   @ ευχαριστω ARM 
    B branch

lower_handler :
    @επειδη αππο [65,90] τα κεφαλαια πας στο [97,122] μικρα αρα 32 διαφορα δηλαδη αλλαζει μονο ενα bit απο μικρα σε κεφαλαια εχεις -32 αρα το bit 5o(ξεκιναω απο 0bit) (2^5 = 32) καντο απο 1->0
    AND R2, R2, #0xDF    @ R2 = R2 AND 11011111 , -32 αρα ετσι απλα
    ADD R12 , R12 , R2 @ hash = hash + R2
    B branch

lut_numbers :
    @DCD 2, 3, 5, 7, 11, 13, 17, 19, 23, 29   @ το λοοκ απ ταβλε
    .word 2, 3, 5, 7, 11, 13, 17, 19, 23, 29 @apla bariomoun na katebasw to keil
        .fnend