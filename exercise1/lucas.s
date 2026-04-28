.global lucas
     .p2align 2
   .type lucas, %function
     
lucas:
    push{lr}                 @store link address in the stack
  bl lucas_recursive       @call the recursive
  pop{pc}                  @back to the main programm


lucas_recursive:
  
    push{lr}
  
  cmp r0, #0  @case zero
  IT EQ
  moveq r0, #2 
  IT EQ 
  popeq{pc}
  
  cmp r0, #1 @case one
  IT EQ
  moveq r0, #1 
  IT EQ
  popeq{pc}
  
  push{r4, r5}
  mov r4, r0               @save n at r4
  sub r0, r0, #1           @ create n-1 at r0
    bl lucas_recursive       @now r0 is the result of the L(n-1)
  mov r5, r0               @save the reult at r5 so now r5 is the result of  L(n-1)
  sub r0, r4, #2           @r0 is now n -2
  bl lucas_recursive       @now r0 is the result of the L(n-2)
  add r0, r0, r5           @now r0 has the L(n-1) + L(n-2)
  pop{r4, r5, pc}