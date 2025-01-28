.orig x3000
AND R0 R0 #0 ;operand 1
AND R1 R1 #0 ;operand 2
AND R2 R2 #0 ;result
AND R3 R3 #0
AND R4 R4 #0 ;used for bitmask
AND R5 R5 #0 ;counter
AND R6 R6 #0 ;addresses
AND R7 R7 #0 ;addresses

LEA R6 operands
LDW R6 R6 #0
LDB R0 R6 #0 ;R0 is operand1
LDB R1 R6 #1 ;R1 is operand2

ADD R5 R5 #8 ;counter

Loop1   BRz finish 
        AND R4 R1 #1 
        Brz Shift
        ADD R2 R2 R0 
Shift   RSHFL R1 R1 #1 
        LSHF R0 R0 #1 
        ADD R5 R5 #-1 
        BRnzp Loop1

finish RSHFL R3 R2 #8 
        BRz Store 
        AND R3 R3 #0 
        ADD R3 R3 #1 

Store LEA R6 result 
        LDW R6 R6 #0
        STB R2 R6 #0
        STB R3 R6 #1

HALT

operands .fill x3100
result .fill x3102

.end
