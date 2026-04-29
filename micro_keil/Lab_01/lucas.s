.global lucas_sequence
    .p2align 2
    .type lucas_sequence, %function


lucas_sequence:
    .fnstart
    @ R0 = Clearance Level (n)
    
    PUSH {LR}               @ Save Link Register because we will call BL
    
    BL lucas_recursive      @ Branch with Link: Call the recursive function
    
    @ When we return here, R0 contains the final Lucas sequence result L(n)
    
    LDR R1, =lucas_mem      @ Load memory address for Lucas result
    STR R0, [R1]            @ Store Register: Save final result to memory
    
    POP {LR}                @ Restore Link Register
    BX LR                   @ Return to C
    .fnend


@anadromiki
    .type lucas_recursive, %function
lucas_recursive:
    .fnstart
    @ Input:  R0 = n
    @ Output: R0 = L(n)
    
    PUSH {R4, LR}           @ Save R4 (variable register) and LR
    
    @ n=0 n=1
    CMP R0, #0              @ Check if n == 0
    BEQ is_zero             @ Branch if Equal: go to zero handler
    
    CMP R0, #1              @ Check if n == 1
    BEQ is_one              @ Branch if Equal: go to one handler
    
    @ --- Recursive Step ---
    @ We need to calculate L(n-1) + L(n-2)
    MOV R4, R0              @ R4 = n (Save n in R4 because BL will overwrite R0)
    
    @ Call L(n-1)
    SUB R0, R4, #1          @ R0 = n - 1
    BL lucas_recursive      @ Recursive Call
    PUSH {R0}               @ Push L(n-1) onto the stack to save it temporarily
    
    @ Call L(n-2)
    SUB R0, R4, #2          @ R0 = n - 2
    BL lucas_recursive      @ Recursive Call
    
    @ Calculate Sum
    POP {R1}                @ Pop L(n-1) from the stack into R1
    ADD R0, R1, R0          @ R0 = L(n-1) + L(n-2)
    
    POP {R4, LR}            @ Restore R4 and LR for the caller
    BX LR                   @ Return
    
@ --- Handlers for Base Cases ---
is_zero:
    MOV R0, #2              @ L(0) = 2
    POP {R4, LR}
    BX LR

is_one:
    MOV R0, #1              @ L(1) = 1
    POP {R4, LR}
    BX LR
    .fnend


@ ==========================================
@ DATA MEMORY SECTION (RAM)
@ ==========================================
    .data
    .p2align 2
lucas_mem:
    .word 0                 @ Allocate 1 word for the final Lucas password