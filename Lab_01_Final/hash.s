	.global hash
    .p2align 2
    .type hash,%function
        
hash:
        .fnstart
        
       
        PUSH {R4, R5 , R6}          @ Push registers onto stack
        
        MOV R1, #0             @ String length counter
        MOV R3, R0             @ Memory pointer for the string
        MOV R6, #0            @ Hash accumulator (sum) 
          
    branch:
        LDRB R2, [R3]          @ Load Register with byte (Read character)
        CMP R2, #0             @ Compare: Check if we reached the end of the string (NULL)
        BEQ end_loop           @ Branch if Equal: If 0, go to end
        ADD R1, R1, #1         @ Add: Increment length
        ADD R3, R3, #1         @ Add: Move memory pointer to next byte

        
        CMP R2, #'0'
        BLT branch             @ Branch if Less Than: If < '0', ignore it
        
        CMP R2, #'9'
        BLE number_handler     @ Branch if Less or Equal: If <= '9', it's a number

        CMP R2, #'A'
        BLT branch             @ Branch if Less Than: If < 'A', ignore it

        CMP R2, #'Z'
        BLE upper_handler      @ Branch if Less or Equal: If <= 'Z', it's uppercase

        CMP R2, #'a'
        BLT branch             @ Branch if Less Than: If < 'a', ignore it

        CMP R2, #'z'
        BLE lower_handler      @ Branch if Less or Equal: If <= 'z', it's lowercase

        B branch               @ If any other symbol, ignore it
            
    end_loop:
        ADD R0, R6, R1        @ Final Hash = Sum (R12) + Length (R1)
              
        
        LDR R2, =hash_mem      @ Load the memory address into R2
        STR R0, [R2]           @ Store Register: Save the final hash (R0) into memory
              
        
        POP {R4, R5 , R6}           @ Pop registers from stack
              
        BX LR                  @ Branch indirect: Return to C function

number_handler:
    SUB R2, R2, #'0'           @ Convert ASCII to integer index
    
    ADR R4, lut_numbers        @ Load PC-relative Address of the LUT
    
    LSL R2, R2, #2             @ Logical Shift Left: R2 = R2 * 4 (32-bit words)
    ADD R4, R4, R2             @ R4 = Base Address + Offset
    LDR R5, [R4]               @ Load Register: Read the value from LUT
    
    ADD R6, R6, R5           @ Add LUT value to Hash
    B branch               

upper_handler:
    LSL R2, R2, #1             @ Logical Shift Left: Multiply ASCII value by 2
    ADD R6, R6, R2           @ Add doubled value to Hash
    B branch

lower_handler:
    AND R2, R2, #0xDF          @ Logical AND: Mask to subtract 32 (lowercase -> uppercase)
    ADD R6, R6, R2           @ Add new ASCII value to Hash
    B branch

    .p2align 2                 @ Memory alignment (Word boundary)
lut_numbers:
    .word 2, 3, 5, 7, 11, 13, 17, 19, 23, 29   @ LUT 32-bit words

    .fnend


    .data                      @ Declare data section
    .p2align 2
hash_mem:
    .word 0                    @ Allocate 1 word (4 bytes) in memory with initial value 0