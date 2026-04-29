.data
      
fmt: .asciz "The value is %d\n"  

     .text
   
   .global checksum
   .extern printf   
   .p2align 2
   .type checksum, %function
     
checksum:
     push {r4, lr}
     mov r1, r0          @move string pointer to r1
   mov r0, #0          @initialize r0
loop_start:   
   ldrb r2, [r1]       @load mem content that r1 points to r2
   cmp r2, #0          @check for end of the string
     beq print
   add r1, r1, #1      @increment pointer to point at the next element
   eor r2, r2, #0xAA   @Xor with 0xAA
   eor r0, r0, r2      @xor the new char with the rest of em
   b loop_start        @go back and do the same for the next char
   
print:

                         @since i am gonna call another func everything i have will be lost
   
   mov r4, r0          @save r4 because print might change the r0-r3
   mov r1, r4          @store the number you want to print at r1 
   ldr r0, =fmt        @load at r1 the string you want to print 
   bl printf           
   mov r0,r4           @restore r0 for this function to be able to return it
   pop {r4, pc}