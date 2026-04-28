.global clearance
     .p2align 2
   .type clearance, %function
     
clearance:
     mov r1, #32        @cntr to check all the bits or r0
   mov r2, #0          @initialize to zero to keep track of one bits sum
    
     
loop:                       
ands r3, r0, #1          @op_and r0 with 1 and store it at r3 to see if lsb is zero or 1
IT NE                    @Cause for some reason the ne and eq are not working without this  
addne r2, r2, #1         @if and result not zero add +1 
lsr r0, r0, #1           @shift r0 to the rigth to check the next bit
subs r1, r1, #1          @i-- and set the flag
bne loop                 @if r1 != 0 go back to loop
                     
level_detect:           @number of 1 bits is stored at r2
mov r1, #6
udiv r3, r2, r1          @now r3 holds the integer number of times six can fit to r2
mls r0, r3, r1, r2       @r0 = r2 - (6*int_Quotient) aka r0 = r2 - (r3*r1)

bx lr