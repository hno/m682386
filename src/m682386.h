#ifndef M682386_H
#define M682386_H

/* FLAGMASK anv„nds i i80306.h */
typedef unsigned char FLAGMASK;

#ifndef M68000_H
#include "m68000.h"
#endif

#ifndef I80386_H
#include "i80386.h"
#endif

#define CONSTANT_SIZE		(90)
#define LABEL_SIZE			(32)
#define COMMENT_SIZE			(100)
#define SOURCE_LINE_SIZE	(100)
#define FILE_NAME_SIZE		(80)
#define INSTRTEXT_SIZE		(30)

typedef enum {
	IS_NOTHING=0,
	IS_SOURCE_LINE,
	IS_INSTR,
	IS_LABEL,
	IS_COMMENT,
	IS_COMMENT_EOL,
	IS_PARSE_ERROR,
	IS_EXP68_ERROR,
	IS_FLAGGOPT_ERROR,
	IS_X_IS_C_ERROR,
	IS_MATCH86_ERROR,
	IS_REGASS_ERROR
} STRUCT_TYPE;

typedef unsigned int LINENUMBER;

typedef unsigned short REGMASK;
typedef union {
	struct {
		unsigned int d0:1,d1:1,d2:1,d3:1,d4:1,d5:1,d6:1,d7:1;
		unsigned int a0:1,a1:1,a2:1,a3:1,a4:1,a5:1,a6:1,a7:1;
	} reg;
	REGMASK mask;
} REGS;

#define REGMASKBIT(reg)	((REGMASK)(1<<(reg-REG_D0)))
#define REGMASKSET(mask,reg) (mask & REGMASKBIT(reg))

#define F_C (1<<0)
#define F_V (1<<1)
#define F_Z (1<<2)
#define F_N (1<<3)
#define F_X (1<<4)
#define F_ALL (F_C|F_V|F_Z|F_N|F_X)
#define F_NONE	(0)

/* vilka flaggor subrutiner antas returnera s„tta, returnera och f”rst”ra */
#define F_SUBSET			(F_Z|F_C|F_N)
#define F_SUBUSE			(F_NONE)
#define F_SUBPRE			(F_NONE)

/* vilka flaggor som kan passera en label  */
#define F_LABEL	(F_NONE)

/* Flagg villkor, vilka flaggor som kr„vs f”r resp villkor */
#define F_HI (F_C | F_Z)
#define F_LS (F_C | F_Z)
#define F_CC (F_C)
#define F_CS (F_C)
#define F_NE (F_Z)
#define F_EQ (F_Z)
#define F_VC (F_V)
#define F_VS (F_V)
#define F_PL (F_N)
#define F_MI (F_N)
#define F_GE (F_N | F_V)
#define F_LT (F_N | F_V)
#define F_GT (F_N | F_V | F_Z)
#define F_LE (F_N | F_V | F_Z)

/* Flagg makron */
#define F_ZCX	(F_Z | F_C | F_X)
#define F_NZVC	(F_N | F_Z | F_V | F_C)
#define F_NVCX	(F_N | F_V | F_C | F_X)
#define F_NZC	(F_N | F_Z | F_C)

typedef enum {
	AM_UNKNOWN=-1,
	AM_NO_OPERAND,
	AM_REG,
	AM_INDIRECT,
	AM_INDIRECT_POSTINC,
	AM_INDIRECT_PREDEC,
	AM_ABS_MEM,
	AM_IMMEDIATE,
	AM_MULTI_REG,
	AM_MEM_INDIRECT,
	AM_SPECIAL
} ADDRMODE;

typedef enum {
	REG_NONE=0,
	REG_D0,
	REG_D1,
	REG_D2,
	REG_D3,
	REG_D4,
	REG_D5,
	REG_D6,
	REG_D7,
	REG_A0,
	REG_A1,
	REG_A2,
	REG_A3,
	REG_A4,
	REG_A5,
	REG_A6,
	REG_A7,
	REG_PC,
	REG_CCR,
	REG_TEMP,
	REG_TEMP2,
	REG_TEMP_INDEX,
	REG_TEMP_MEMIND1,
	REG_TEMP_MEMIND2,
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_EBP,
	REG_ESI,
	REG_EDI,
	REG_ESP,
	REG_68D_BASE=REG_D0,
	REG_68A_BASE=REG_A0,
	REG_68BASE=REG_D0,
	REG_SP=REG_A7,
	REG_IP=REG_PC,

} REGISTER;

#define ISTEMPREG(n)		(n>=REG_TEMP && n<=REG_TEMP_MEMIND2)
#define REG_68DATA(n)	(REG_68D_BASE+(n))
#define REG_68ADDR(n)	(REG_68A_BASE+(n))
#define REG_68(n)		(REG_68BASE+(n))

typedef struct {
	char text[CONSTANT_SIZE+1];
} CONSTANT;

typedef struct {
	char text[INSTRTEXT_SIZE+1];
} INSTRTEXT;

typedef struct {
	char text[LABEL_SIZE+1];
} LABEL;

typedef struct {
	char text[COMMENT_SIZE+1];
} COMMENT;

typedef struct {
	char text[SOURCE_LINE_SIZE];
	LINENUMBER lineNumber;
	char fileName[FILE_NAME_SIZE];
} SOURCE_LINE;

typedef enum {
	SIZE_UNKNOWN=0,
	SIZE_NONE=-1,
	SIZE_B=1,
	SIZE_W=2,
	SIZE_L=4
} SIZE;

/* hur hanteras denna operand? */
typedef enum {
	OP_UNKNOWN,
	OP_NO_OPERAND,
	OP_READ,
	OP_WRITE,
	OP_MODIFY,
	OP_FLOW,
	OP_DATA,
	OP_MACRO,
	OP_SPECIAL
} OPMODE;

typedef struct {
	ADDRMODE addrMode;	/* Addresserings mode */
	CONSTANT constant;	/* Displacement eller konstant mm */
	REGISTER reg;			/* Bas/huvud register */
	REGISTER index;		/* Yttre Index register */
	REGISTER outerIndex;	/* Yttre index i memory indirect */
	CONSTANT outerDispl; /* Yttre displacement i memory indirect */
	SIZE indexSize;
	int indexScale;
	REGS regs;				/* Register mask i multireg */
	SIZE size;				/* Operand storlek */
	OPMODE mode;			/* Operand mode */
} OPERAND;

/* Hur hanteras operanderna ? */
typedef enum {
	O_UNKNOWN,	/* Ok„nd... */
	O_SPECIAL,	/* Special special (hmm... inte vet jag) */
	O_NONE,		/* Inga operander */
	O_NORMAL,	/* Tv† operander: k„lla,destination */
	O_SRC,		/* Operanden „r k„lla */
	O_DST,		/* Operanden „r destination */
	O_MOD,		/* Operanden modifieras */
	O_MOD2,		/* Operanderna modifieras */
	O_MDST,		/* Destinations operanden modifieras */
	O_MDST1,		/* En till tv† operander, dest modifieras */
	O_FLOW,		/* En operand som „r label */
	O_FLOW2,		/* register,label (DBcc) */
	O_CHECK,		/* Tv† k„ll operander */
	O_DATA,		/* Instruktionen „r ett data direktiv */
	O_MACRO		/* Instruktionen „r ett macro */
} OPSMODE;

/* struktur f”r 68000 instruktion */
typedef struct {
	INSTRCODE68 instr;
	INSTRTEXT instrText;
	SIZE size;
	OPERAND op1,op2;
	OPSMODE opsMode;
} INSTR68;

/* struktur f”r 80386 instruktion */
typedef struct {
	INSTRCODE86 instr;
   INSTRTEXT instrText;
	SIZE size;
	OPERAND op1,op2;
	OPSMODE opsMode;
} INSTR86;

extern char objectName[];
extern char objectPath[];

#endif /* M682386_H */
