;
; This program takes two inputs in succession, between -999 and 999, stores them
; in R1 and R2 after processing of input and then calculates the relation between
; the two numbers, prints the value of R0 and then sets the correct value into R0,
; the value of R0 being one of the below (mentioned in Calculation Method).
;
; NOTE: The intended endpoints of the application are at addresses: x3090, x3096,
;	x30A4, x30AA and x30AF.
;


; Calculation Method
; First step, check on what side of 0 R1 and R2 are
;   If R1 < 0, and R2 >= 0, then R1 < R2 so R0 <== -1
;   If R1 >= 0, and R2 < 0, then R1 > R2 so R0 <== 1
;   If R1 < 0, and R2 < 0, or If R1 >= 0, and R2 >= 0, Then:
; The calculation R3 <== R1 - R2 will be performed.
;   If R3 -ve, then R1 < R2 so R0 <== -1
;   If R3 +ve, then R1 > R2 so R0 <== 1
;   If R0 = 0, then R1 = R2 so R0 <== 0
; This method is used to ensure that an overflow into R3 does not occur.


; Register Usage :
;	R0 : final result/Input from GETC
;	R1 : First Number
;	R2 : Second Number
;	R3 : Result from calculation if needed
;	R4 : Null Character/minus Character
;	R5 : Top of Input_Stack
;	R6 : Pointer To Lookup Table

.ORIG x3000				; Origin Memory


CREATE_NULL:
	ADD R4, R4, #10  		; Newline character has ASCII value of 10 (dec) 0xA (hex)
	NOT R4, R4  			; 2's compliment negation of newline value for comparison
	ADD R4, R4, #1  		; ^^^^^^^^^^^^^^^^^^^^^^^^^^^


CREATE_STACK:
	LEA R5, INPUT_STACK		; Make R5 Point to the top of the InputStack
	ADD R3, R3, #10			; Create an illegal stack value of 10
	STR R3, R5, #0			; Store the illegal value so we know where the top of the stack is
	ADD R5, R5, #1			; Increment by one so we know when to stop when creating number


GET_R1:
	GETC				; Get Next Digit
	OUT				; Prints character to the console
	AND R3, R3, #0			; Clear R3
	ADD R3, R0, R4			; Check to see if null character was selected
	BRz PROCESS_R1			; Go and process the stack
	ADD R0, R0, #-15		; Convert from ASCII to "decimal"
	ADD R0, R0, #-15		; Convert from ASCII to "decimal"
	ADD R0, R0, #-15		; Convert from ASCII to "decimal"
	ADD R0, R0, #-3			; Convert from ASCII to "decimal"
	STR R0, R5, #0			; Put digit onto the stack
	ADD R5, R5, #1			; Increment Stack Counter
	BR GET_R1			; Loop back


PROCESS_R1:
	ADD R5, R5, #-1			; Deincrement stack counter as it one too much ahead
	LDR R3, R5, #0			; Load the value
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #-10		; Check to see if top of stack is reached
	BRz GET_R2			; Branch to get R2 if top of stack was reached
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #0			; Check to see if a minus sign has been read, due to conversion a minus sign will be -3, which is an illegal value
	BRn MAKE_R1_NEGATIVE		; If minus sign has been reached, then go and Make R1 negative
	ADD R1, R1, R3			; Add the value from the top of the stack to R1

	ADD R5, R5, #-1			; Deincrement stack counter
	LDR R3, R5, #0			; Load the value
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #-10		; Check to see if top of stack is reached
	BRz GET_R2			; Branch to get R2 if top of stack was reached
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #0			; Store Value in R0
	ADD R0, R3, #0			; Check to see if a minus sign has been read, due to conversion a minus sign will be -3, which is an illegal value
	BRn MAKE_R1_NEGATIVE		; If minus sign has been reached, then go and Make R1 negative
	LEA R6, LookUp10		; Make R6 point to the lookup10 table
	ADD R6, R6, R3			; Point at correct factor in LookUp10
	AND R0, R0, #0			; Clear R0
	LDR R0, R6, #0			; Load factor into R0
	ADD R1, R1, R0			; Add factor to total in R1

	ADD R5, R5, #-1			; Deincrement stack counter
	LDR R3, R5, #0			; Load the value
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #-10		; Check to see if top of stack is reached
	BRz GET_R2			; Branch to get R2 if top of stack was reached
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #0			; Store Value in R0
	ADD R0, R3, #0			; Check to see if a minus sign has been read, due to conversion a minus sign will be -3, which is an illegal value
	BRn MAKE_R1_NEGATIVE		; If minus sign has been reached, then go and Make R1 negative
	LEA R6, LookUp100		; Make R6 point to the lookup100 table
	ADD R6, R6, R3			; Point at correct factor in LookUp100
	AND R0, R0, #0			; Clear R0
	LDR R0, R6, #0			; Load factor into R0
	ADD R1, R1, R0			; Add factor to total in R1

	ADD R5, R5, #-1			; Deincrement stack counter
	LDR R3, R5, #0			; Load the value
	AND R0, R0, #0			; Clear R0
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #0			; Store Value in R0
	ADD R0, R3, #0			; Check to see if a minus sign has been read, due to conversion a minus sign will be -3, which is an illegal value
	BRn MAKE_R1_NEGATIVE		; If minus sign has been reached, then go and Make R1 negative
	BR GET_R2			; Computation of R1 complete, go and get R2
	ADD R5, R5, #1			; Increment stack counter


MAKE_R1_NEGATIVE:
	NOT R1, R1			; 2's compliment negation of R1
	ADD R1, R1, #1			; ^^^^^^^^^^^^^^^^^^^^^^
	STR R5, R5, #0			; Clear the Value at the top of the stack


GET_R2:
	ADD R5, R5, #1			; Increment stack counter, as it is currently pointing at the end of stack character


GET_R2_LOOP:
	GETC				; Get Next Digit
	OUT				; Prints character to the console
	AND R3, R3, #0			; Clear R3
	ADD R3, R0, R4			; Check to see if null character was selected
	BRz PROCESS_R2			; Go and process the stack
	ADD R0, R0, #-15		; Convert from ASCII to "decimal"
	ADD R0, R0, #-15		; Convert from ASCII to "decimal"
	ADD R0, R0, #-15		; Convert from ASCII to "decimal"
	ADD R0, R0, #-3			; Convert from ASCII to "decimal"
	STR R0, R5, #0			; Put digit onto the stack
	ADD R5, R5, #1			; Increment Stack Counter
	BR GET_R2_LOOP			; Loop back


PROCESS_R2:
	ADD R5, R5, #-1			; Deincrement stack counter as it one too much ahead
	LDR R3, R5, #0			; Load the value
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #-10		; Check to see if top of stack is reached
	BRz CHECKING_LOOP		; Branch to check both numbers if top of stack was reached
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #0			; Store Value in R0
	ADD R0, R3, #0			; Check to see if a minus sign has been read, due to conversion a minus sign will be -3, which is an illegal value
	BRn MAKE_R2_NEGATIVE		; If minus sign has been reached, then go and Make R1 negative
	ADD R2, R2, R3			; Add the value from the top of the stack to R2

	ADD R5, R5, #-1			; Deincrement stack counter
	LDR R3, R5, #0			; Load the value
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #-10		; Check to see if top of stack is reached
	BRz CHECKING_LOOP		; Branch to check both numbers if top of stack was reached
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #0			; Store Value in R0
	ADD R0, R3, #0			; Check to see if a minus sign has been read, due to conversion a minus sign will be -3, which is an illegal value
	BRn MAKE_R2_NEGATIVE		; If minus sign has been reached, then go and Make R1 negative
	LEA R6, LookUp10		; Make R6 point to the lookup10 table
	ADD R6, R6, R3			; Point at correct factor in LookUp10
	AND R0, R0, #0			; Clear R0
	LDR R0, R6, #0			; Load factor into R0
	ADD R2, R2, R0			; Add factor to total in R2

	ADD R5, R5, #-1			; Deincrement stack counter
	LDR R3, R5, #0			; Load the value
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #-10		; Check to see if top of stack is reached
	BRz CHECKING_LOOP		; Branch to check both numbers if top of stack was reached
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #0			; Check to see if a minus sign has been read, due to conversion a minus sign will be -3, which is an illegal value
	BRn MAKE_R2_NEGATIVE		; If minus sign has been reached, then go and Make R1 negative
	LEA R6, LookUp100		; Make R6 point to the lookup100 table
	ADD R6, R6, R3			; Point at correct factor in LookUp100
	AND R0, R0, #0			; Clear R0
	LDR R0, R6, #0			; Load factor into R0
	ADD R2, R2, R0			; Add factor to total in R2

	ADD R5, R5, #-1			; Deincrement stack counter
	LDR R3, R5, #0			; Load the value
	AND R0, R0, #0			; Clear R0
	ADD R0, R3, #0			; Check to see if a minus sign has been read, due to conversion a minus sign will be -3, which is an illegal value
	BRn MAKE_R2_NEGATIVE		; If minus sign has been reached, then go and Make R2 negative
	BR CHECKING_LOOP		; Go and compare the two numbers


MAKE_R2_NEGATIVE:
	NOT R2, R2			; 2's compliment negation of R2
	ADD R2, R2, #1			; ^^^^^^^^^^^^^^^^^^^^^^
	STR R5, R5, #0			; Clear the Value at the top of the stack


CHECKING_LOOP:
	ADD R3, R1, #0			; Check if R1 is negative or positive
	BRn R1_NEGATIVE			; If negative, then branch to R1_negative Branch
	AND R3, R3, #0			; Clear value in R3
	ADD R3, R2, #0			; Check if R2 is negative or positive
	BRn R1_POSITIVE_R2_NEGATIVE	; At this point it is known that R1 is positive or 0 and R2 is negative
	AND R3, R3, #0			; Clear value in R3
	BR R1_R2_SAME_SIGN		; If neither previous branch was triggered, it means that both R1 and R2 are positive


R1_NEGATIVE:
	AND R3, R3, #0			; Clear value in R3
	ADD R3, R2, #0			; Check if R2 is negative or positive
	BRn R1_R2_SAME_SIGN		; If R2 Is negative, then both R1 and R2 are negative
	AND R3, R3, #0			; Clear value in R3
	BR R1_NEGATIVE_R2_POSITIVE	; If the previous branch was not triggered, then R1 is negative and R2 is positive or 0


R1_NEGATIVE_R2_POSITIVE:
	AND R0, R0, #0			; Make sure value in R0 is clear
	LEA R0, NEGATIVE_ONE		; Make R0 point to top of negative result string
	PUTS				; Print Result
	AND R0, R0, #0			; Make sure value in R0 is clear
	ADD R0, R0, #-1			; R1 < R2, thus R0 <== -1
	HALT				; End Program


R1_POSITIVE_R2_NEGATIVE:
	AND R0, R0, #0			; Make sure value in R0 is clear
	LEA R0, POSITIVE_ONE		; Make R0 point to top of positive result string
	PUTS				; Print result
	AND R0, R0, #0			; Make sure value in R0 is clear
	ADD R0, R0, #1			; R1 > R2, thus R0 <== 1
	HALT				; End Program


R1_R2_SAME_SIGN:
	NOT R2, R2			; 2's Compliment Negation of R2
	ADD R2, R2, #1			; ^^^^^^^
	AND R3, R3, #0			; Clear value in R3
	ADD R3, R1, R2			; Subtraction of R2 from R1
	BRn NEGATIVE_LOOP		; if result is negative then R1 < R2
	AND R3, R3, #0			; Clear value in R3
	ADD R3, R1, R2			; Subtraction of R2 from R1
	BRp POSITIVE_LOOP		; If Result was positive, then R1 > R2
	BR ZERO_LOOP			; If neither branch was triggered, then R1 = R2


POSITIVE_LOOP:
	;AND R0, R0, #0			; Clear value in R0
	LEA R0, POSITIVE_ONE		; Make R0 point to top of positive result string
	PUTS				; Print result
	AND R0, R0, #0			; Clear value in R0
	ADD R0, R0, #1			; If Result was positive, then R1 > R2, thus R0 <== 1
	HALT				; End Program


NEGATIVE_LOOP:
	AND R0, R0, #0			; Clear value in R0
	LEA R0, NEGATIVE_ONE		; Make R0 point to top of negative result string
	PUTS				; Print Result
	AND R0, R0, #0			; Clear value in R0
	ADD R0, R0, #-1			; If Result was Negative, then R1 < R2, thus R0 <== -1
	HALT				; End Program


ZERO_LOOP:
	AND R0, R0, #0			; Clear value in R0
	LEA R0, ZERO			; Make R0 point to top of zero result string
	PUTS				; print string
	AND R0, R0, #0			; Clear value in R0, This sets R0 to 0, as R1 = R2
	HALT				; End Program


INPUT_STACK:
	.BLKW #5			; Reserve 5 memory spaces for input number


LookUp10:     				; Lookuptable for base 10 values
	.FILL  #0
	.FILL  #10
	.FILL  #20
	.FILL  #30
	.FILL  #40
	.FILL  #50
	.FILL  #60
	.FILL  #70
	.FILL  #80
	.FILL  #90


LookUp100      				; Lookuptable for base 100 values
	.FILL  #0
	.FILL  #100
	.FILL  #200
	.FILL  #300
	.FILL  #400
	.FILL  #500
	.FILL  #600
	.FILL  #700
	.FILL  #800
	.FILL  #900


NEGATIVE_ONE:
	.STRINGZ "-1"


POSITIVE_ONE:
	.STRINGZ "1"

ZERO:
	.STRINGZ "0"


.END
