.386p
jumps
amiga segment para public 'AMIGA'
assume cs:amiga
assume ds:amiga
assume ss:amiga

REG68 UNION
B	DB ?
W	DW ?
L	DD ?
REG68 ENDS

D0      REG68 ?
D1      REG68 ?
D2      REG68 ?
D3      REG68 ?
D4      REG68 ?
D5      REG68 ?
D6      REG68 ?
D7      REG68 ?
A0      REG68 ?
A1      REG68 ?
A2      REG68 ?
A3      REG68 ?
A4      REG68 ?
A5      REG68 ?
A6      REG68 ?


CLR MACRO EA
	MOV     EA,0
ENDM

RSTRUCRESET macro
	RSCOUNTER=0
ENDM

RSTRUCSET MACRO CNT
	RSCOUNTER=CNT
ENDM

RSTRUCB MACRO CNT
	RSCOUNTER=RSCOUNTER+(CNT*1)
ENDM

RSTRUCW MACRO CNT
	RSCOUNTER=RSCOUNTER+(CNT*2)
ENDM

RSTRUCD MACRO CNT
	RSCOUNTER=RSCOUNTER+(CNT*4)
ENDM
