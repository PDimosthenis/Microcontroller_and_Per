	.global clearence
    .p2align 2
    .type clearence, %function

clearence:
    .fnstart    
    MOV R1, #0             @ initialize the ones counter to 0

@Hamming Weight Calculation
count_ones_loop:
    CMP R0, #0             @ check if the hash has become 0
    BEQ calculate_mod      @ branch if equal if yes no more 1 so go to calculate mod
    
    LSRS R0, R0, #1        @ LSRS pushes the lsb bit into the Carry Flag
    ADC R1, R1, #0         @ add with carry: R1 = R1 + 0 + Carry
                           @ if carry = 1 then R1++; else R1 ;
    
    B count_ones_loop      @ branch back

@ mod 6 with substructions
calculate_mod:
    CMP R1, #6             @ compare the counter with 6
    BLT store_result       @ branch if less than if R1 < 6 the remainder is R1 itself
    
    SUB R1, R1, #6         @ else keep subtracting 6 (R1 = R1 - 6)
    B calculate_mod        @ branch

@store result and retrun
store_result:
    LDR R2, =clearance_mem @ load the memory address into R2
    STR R1, [R2]           @ store the value of R1 in memory
    
    MOV R0, R1             @ move the final Clearance Level (0-5) to R0
    
    BX LR                  @ return to main
    .fnend



    .data                 
    .p2align 2
clearance_mem:
    .word 0                @1 word 4bytes init 0 to store clearence