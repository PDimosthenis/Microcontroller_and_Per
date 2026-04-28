.data
look_up_table:
    .word 2, 3, 5, 7, 11, 13, 17, 19, 23, 29  

    .text
    
    .global hash            
    .p2align 2              
    .type hash, %function   

hash:
    
    push {r4, LR}
@Free ro to start saving the hash result and initialize with zero
    mov r1, r0 
    mov r0, #0
  ldr r4, =look_up_table  @r4 points to the first element of look_up

loop_start: 
    ldrb r2, [r1, #0]   @From now on r2 contains the value of each character
  cmp r2, #0          @check if we are at the end of the string n
    add r1, r1, #1    @move ptr to the next memory position for next char    
    beq loop_end
  
  add r0, r0,#1       @The initial hash value is the size of the string so each time we add +1 no matter what
  cmp r2, #48         @48 is ascii for 0 
  blt loop_start      @no number or letter just go to next iteration
  cmp r2, #57         @57 is ascii for 9
  bgt uppercase       @if greater go check for uppercase otherwise its number so do the followin
  sub r2, r2, #48     @numbers in ascii start from 48 so we subtract in order to use r2 as index for look_up
  lsl r2, r2, #2      @do it x4 caue look_up uses words
  
  ldr r3, [r4, r2]    @load the look_up[i] at r3
  add r0, r0 ,r3      @add the number to the hash value
  b loop_start        @no condition just jump at the begging of the loop
  
uppercase:              @we are sent here because r2 is greater than 57
    cmp r2, #65
  blt loop_start      @means we are inbetween 57 and 65 so do nothing
  cmp r2, #90
  bgt lowercase       @if r2 greater than 90 we might have a lowercase case :)
  lsl r2, r2, #1      @we still here so we mult by 2 the ascii
  add r0, r0 , r2     @and add the result to the hash
  b loop_start        @on to the next rep

lowercase:              @we are sent here because r2 is  greater than 90
    cmp r2, #97         
  blt loop_start      @means we are in between 90 and 97 so do nothing
  cmp r2, #122        
  bgt loop_start      @i am past the lowecase letters so go to the next rep
  mov r3, #1
  lsl r3, r3, #5      @difference from lowercase to uppercase is 32 so 100000
  mvn r3, r3          @reverse so in the bitwise and the 6th bit 32 is gonna turn zero while the rest will stay as they are
  and r2, r2, r3
  add r0, r0, r2
  b loop_start 
  
loop_end:
    pop {r4,  PC}    @pop r4 and give pc the lr value so we can continue outside of the func in the main