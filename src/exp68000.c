/* exp68000
 * expand 68000 instructions and flags
 */

#include <stdlib.h>
#include <stdio.h>

#include "m682386.h"
#include "parser.h"
#include "exp68000.h"

#define MAX_OUTPUT	5

/* from where are we reading input */
#define READER parse
typedef PARSED INPUT;

typedef EXP68000 OUTPUT;

typedef OUTPUT *EXPFUNK(OUTPUT *,INSTR68 *);

/* Output queue */
static OUTPUT out[MAX_OUTPUT];
static int nitems=0;
static int curritem=0;

static void output(OUTPUT *);

/* Add one "instruction" to the output queue */
static void output(OUTPUT *elem)
{
	if(nitems>=MAX_OUTPUT) {
		fprintf(stderr,"MATCH86: Output queue full in line %d\n",elem->lineNumber);
		exit(1);
	}
	out[nitems++]=*elem;
}

/* Reset output queue */
static void resetOutput(void)
{
	if(curritem<nitems) {
		fprintf(stderr,"MATCH86: resetOutput on nonempty output queue!!! lastline:%d\n"
			,out[nitems-1].lineNumber);
		exit(1);
	}
	/* OK. The ouput queue is empty */
	curritem=0;
	nitems=0;
}

/* Definitions for 68000 flag lookup table */
typedef struct exp68tab EXP68TAB;
struct exp68tab {
  INSTRCODE68 instr;
  FLAGMASK flagsSet,flagsUsed,flagsPreserved;
  OPSMODE opsMode;
  SIZE defSize;
  EXPFUNK *funk;
  INSTRCODE68 realInstr;
};

/* Some special cases */
static EXPFUNK exprAddr;
static EXPFUNK exprCCR;
static EXPFUNK exprAddrCCR;

/********** flags for 68000 *********/
static EXP68TAB exp68tab[] = {
	/* nstr		 sets 	 uses	 preserv operands size		special	instr*/
	{INSTR68_ABCD	,F_C|F_X,F_X	,F_NONE	,O_MDST	,SIZE_B},
	{INSTR68_ADD	,F_ALL	,F_NONE	,F_NONE	,O_MDST	,SIZE_UNKNOWN	,exprAddr},
	{INSTR68_ADDX	,F_C|F_X,F_X	,F_NONE	,O_MDST},
	{INSTR68_ADDQ	,F_ALL	,F_NONE	,F_NONE	,O_MDST	,SIZE_L   	,NULL	,INSTR68_ADD},
	{INSTR68_AND	,F_NZVC	,F_NONE	,F_NONE	,O_MDST	,SIZE_UNKNOWN	,exprCCR},
	{INSTR68_ASL	,F_ALL	,F_NONE	,F_NONE	,O_MDST1},
	{INSTR68_ASR	,F_ALL	,F_NONE	,F_NONE	,O_MDST1},
	{INSTR68_BRA	,F_NONE	,F_NONE	,F_NONE	,O_FLOW},
	{INSTR68_BHI	,F_NONE	,F_HI	,F_ALL	,O_FLOW},
	{INSTR68_BLS	,F_NONE	,F_LS	,F_ALL	,O_FLOW},
	{INSTR68_BCC	,F_NONE	,F_CC	,F_ALL	,O_FLOW},
	{INSTR68_BCS	,F_NONE	,F_CS	,F_ALL	,O_FLOW},
	{INSTR68_BNE	,F_NONE	,F_NE	,F_ALL	,O_FLOW},
	{INSTR68_BEQ	,F_NONE	,F_EQ	,F_ALL	,O_FLOW},
	{INSTR68_BVC	,F_NONE	,F_VC	,F_ALL	,O_FLOW},
	{INSTR68_BVS	,F_NONE	,F_VS	,F_ALL	,O_FLOW},
	{INSTR68_BPL	,F_NONE	,F_PL	,F_ALL	,O_FLOW},
	{INSTR68_BMI	,F_NONE	,F_MI	,F_ALL	,O_FLOW},
	{INSTR68_BGE	,F_NONE	,F_GE	,F_ALL	,O_FLOW},
	{INSTR68_BLT	,F_NONE	,F_LT	,F_ALL	,O_FLOW},
	{INSTR68_BGT	,F_NONE	,F_GT	,F_ALL	,O_FLOW},
	{INSTR68_BLE	,F_NONE	,F_LE	,F_ALL	,O_FLOW},
	{INSTR68_BCHG	,F_Z	,F_NONE	,F_NVCX	,O_MDST	,SIZE_L},
	{INSTR68_BCLR	,F_Z	,F_NONE	,F_NVCX	,O_MDST	,SIZE_L},
	{INSTR68_BFCHG	,F_NZVC	,F_NONE	,F_X	,O_MOD},
	{INSTR68_BFCLR	,F_NZVC	,F_NONE	,F_X	,O_MOD},
	{INSTR68_BFEXTS,F_NZVC	,F_NONE	,F_X	,O_NORMAL},
	{INSTR68_BFEXTU,F_NZVC	,F_NONE	,F_X	,O_NORMAL},
	{INSTR68_BFFFO	,F_NZVC	,F_NONE	,F_X	,O_NORMAL},
	{INSTR68_BFINS	,F_NZVC	,F_NONE	,F_X	,O_MDST},
	{INSTR68_BFSET	,F_NZVC	,F_NONE	,F_X	,O_MOD},
	{INSTR68_BFTST	,F_NZVC	,F_NONE	,F_X	,O_SRC},
	{INSTR68_BSET	,F_Z	,F_NONE	,F_NVCX	,O_MDST	,SIZE_L},
	{INSTR68_BSR	,F_SUBSET,F_SUBUSE,F_SUBPRE,O_FLOW},
	{INSTR68_BTST	,F_Z	,F_NONE	,F_NVCX	,O_CHECK,SIZE_L},
	{INSTR68_CLR	,F_NZVC	,F_NONE	,F_X	,O_DST},
	{INSTR68_CMP	,F_NZVC	,F_NONE	,F_X	,O_CHECK},
	{INSTR68_CMP2	,F_Z|F_C,F_NONE	,F_N|F_V,O_CHECK},
	{INSTR68_DBT	,F_NONE	,F_NONE	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBF	,F_NONE	,F_NONE	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBHI	,F_NONE	,F_HI	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBLS	,F_NONE	,F_LS	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBCC	,F_NONE	,F_CC	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBCS	,F_NONE	,F_CS	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBNE	,F_NONE	,F_NE	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBEQ	,F_NONE	,F_EQ	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBVC	,F_NONE	,F_VC	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBVS	,F_NONE	,F_VS	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBPL	,F_NONE	,F_PL	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBMI	,F_NONE	,F_MI	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBGE	,F_NONE	,F_GE	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBLT	,F_NONE	,F_LT	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBGT	,F_NONE	,F_GT	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DBLE	,F_NONE	,F_LE	,F_ALL	,O_FLOW2,SIZE_NONE},
	{INSTR68_DIVS	,F_NZVC	,F_NONE	,F_X	,O_MDST	,SIZE_W},
	{INSTR68_DIVSL	,F_NZVC	,F_NONE	,F_X	,O_MDST},
	{INSTR68_DIVU	,F_NZVC	,F_NONE	,F_X	,O_MDST	,SIZE_W},
	{INSTR68_DIVUL	,F_NZVC	,F_NONE	,F_X	,O_MDST},
	{INSTR68_EOR	,F_NZVC	,F_NONE	,F_ALL	,O_MDST	,SIZE_UNKNOWN	,exprCCR},
	{INSTR68_EXG	,F_NONE	,F_NONE	,F_ALL	,O_MOD2},
	{INSTR68_EXT	,F_NZVC	,F_NONE	,F_X	,O_MOD},
	{INSTR68_EXTB	,F_NZVC	,F_NONE	,F_X	,O_MOD},
	{INSTR68_JMP	,F_NONE	,F_NONE	,F_ALL	,O_FLOW},
	{INSTR68_JSR	,F_SUBSET,F_SUBUSE,F_SUBPRE,O_FLOW},
	{INSTR68_LEA	,F_NONE	,F_NONE	,F_ALL	,O_NORMAL,SIZE_NONE},
	{INSTR68_LINK	,F_NONE	,F_NONE	,F_ALL	,O_NORMAL,SIZE_NONE},
	{INSTR68_LSL	,F_ALL	,F_NONE	,F_NONE	,O_MDST1},
	{INSTR68_LSR	,F_ALL	,F_NONE	,F_NONE	,O_MDST1},
	{INSTR68_MOVE	,F_NZVC	,F_NONE	,F_X	,O_NORMAL,SIZE_UNKNOWN	,exprAddrCCR},
	{INSTR68_MOVEQ	,F_NZVC	,F_NONE	,F_X	,O_NORMAL,SIZE_L   	,NULL	,INSTR68_MOVE},
	{INSTR68_MOVEM	,F_NONE	,F_NONE	,F_ALL	,O_NORMAL},
	{INSTR68_MULS	,F_NZVC	,F_NONE	,F_X	,O_MDST	,SIZE_W},
	{INSTR68_MULU	,F_NZVC	,F_NONE	,F_X	,O_MDST	,SIZE_W},
	{INSTR68_NBCD	,F_ZCX	,F_X	,F_NONE	,O_MOD},
	{INSTR68_NEG	,F_ALL	,F_NONE	,F_NONE	,O_MOD},
	{INSTR68_NEGX	,F_ALL	,F_X	,F_NONE	,O_MOD},
	{INSTR68_NOP	,F_NONE	,F_NONE	,F_ALL	,O_NONE},
	{INSTR68_NOT	,F_NZVC	,F_NONE	,F_X	,O_MOD},
	{INSTR68_OR	,F_NZVC	,F_NONE	,F_X	,O_NORMAL,SIZE_UNKNOWN	,exprCCR},
	{INSTR68_PEA	,F_NONE	,F_NONE	,F_ALL	,O_SRC	,SIZE_NONE},
	{INSTR68_ROL	,F_NZVC	,F_NONE	,F_X	,O_MDST1},
	{INSTR68_ROR	,F_NZVC	,F_NONE	,F_X	,O_MDST1},
	{INSTR68_ROXL	,F_ALL	,F_X	,F_NONE	,O_MDST1},
	{INSTR68_ROXR	,F_ALL	,F_X	,F_NONE	,O_MDST1},
	{INSTR68_RTD	,F_NONE	,F_SUBSET,F_NONE,O_SRC},
	{INSTR68_RTR	,F_NONE	,F_NONE	,F_NONE	,O_NONE},
	{INSTR68_RTS	,F_NONE	,F_NONE	,F_NONE	,O_NONE},
	{INSTR68_SBCD	,F_C|F_X,F_X	,F_NONE	,O_MDST	,SIZE_B},
	{INSTR68_ST	,F_NONE	,F_NONE	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SF	,F_NONE	,F_NONE	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SHI	,F_NONE	,F_HI	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SLS	,F_NONE	,F_LS	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SCC	,F_NONE	,F_CC	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SCS	,F_NONE	,F_CS	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SNE	,F_NONE	,F_NE	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SEQ	,F_NONE	,F_EQ	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SVC	,F_NONE	,F_VC	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SVS	,F_NONE	,F_VS	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SPL	,F_NONE	,F_PL	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SMI	,F_NONE	,F_MI	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SGE	,F_NONE	,F_GE	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SLT	,F_NONE	,F_LT	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SGT	,F_NONE	,F_GT	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SLE	,F_NONE	,F_LE	,F_ALL	,O_MOD	,SIZE_B},
	{INSTR68_SUB	,F_ALL	,F_NONE	,F_NONE	,O_MDST	,SIZE_UNKNOWN	,exprAddr},
	{INSTR68_SUBQ	,F_ALL	,F_NONE	,F_NONE	,O_MDST	,SIZE_L		,NULL	,INSTR68_SUB},
	{INSTR68_SUBX	,F_C|F_X,F_X	,F_NONE	,O_MDST	,SIZE_UNKNOWN	,exprAddr},
	{INSTR68_SWAP	,F_NZVC	,F_NONE	,F_X	,O_MOD	,SIZE_L},
	{INSTR68_TST	,F_NZVC	,F_NONE	,F_X	,O_SRC},
	{INSTR68_UNLK	,F_NONE	,F_NONE	,F_ALL	,O_SRC},
	/* Devpac special */
	{INSTR68_PUSH	,F_NONE	,F_NONE	,F_ALL	,O_SRC	,SIZE_L},
	{INSTR68_PULL	,F_NONE	,F_NONE	,F_ALL	,O_DST	,SIZE_L},
	/* 682386 special */
	{INSTR68_CLR_X	,F_X	,F_NONE	,F_NONE	,O_NONE},
	{INSTR68_RTKF	,F_NONE	,F_SUBSET,F_NONE,O_NONE},
	/* Data directives */
	{INSTR68_DC	,F_NONE	,F_NONE	,F_NONE	,O_DATA},
	{INSTR68_DCB	,F_NONE	,F_NONE	,F_NONE	,O_DATA},
	{INSTR68_DS	,F_NONE	,F_NONE	,F_NONE	,O_DATA},
	{INSTR68_INCBIN,F_NONE	,F_NONE	,F_NONE	,O_SPECIAL,SIZE_NONE},
	{INSTR68_EVEN	,F_NONE	,F_NONE	,F_NONE	,O_NONE	,SIZE_NONE},
	{INSTR68_CNOP	,F_NONE	,F_NONE	,F_NONE	,O_DATA	,SIZE_NONE},
	/* RS directoves */
	{INSTR68_RS	,F_NONE	,F_NONE	,F_NONE	,O_DATA},
	{INSTR68_RSRESET,F_NONE	,F_NONE	,F_NONE	,O_NONE	,SIZE_NONE},
	{INSTR68_RSSET	,F_NONE	,F_NONE	,F_NONE	,O_MACRO,SIZE_NONE},
	/* "label" directives */
	{INSTR68_EQU	,F_NONE	,F_NONE	,F_NONE	,O_DATA	,SIZE_NONE},
	{INSTR68_SET	,F_NONE	,F_NONE	,F_ALL	,O_DATA	,SIZE_NONE},
	/* Macros */
	{INSTR68_MACRO_USE,F_NONE,F_NONE,F_NONE	,O_MACRO,SIZE_NONE},

	/* Useful assembly directives */
	{INSTR68_IFNE	,F_NONE	,F_NONE	,F_NONE	,O_DATA	,SIZE_NONE},
	{INSTR68_ENDC	,F_NONE	,F_NONE	,F_NONE	,O_NONE	,SIZE_NONE},
	{INSTR68_REPT	,F_NONE	,F_NONE	,F_NONE	,O_DATA	,SIZE_NONE},
	{INSTR68_ENDR	,F_NONE	,F_NONE	,F_NONE	,O_NONE	,SIZE_NONE},
	/* Useful but not useful in translation */
	{INSTR68_INCLUDE,F_NONE	,F_NONE	,F_NONE	,O_SPECIAL,SIZE_NONE},
	{INSTR68_MACRO	,F_NONE	,F_NONE	,F_NONE	,O_MACRO,SIZE_NONE},
	{INSTR68_ENDM	,F_NONE	,F_NONE	,F_NONE	,O_MACRO,SIZE_NONE},
	{INSTR68_STRUCT,F_NONE	,F_NONE	,F_NONE	,O_MACRO,SIZE_NONE},
	{INSTR68_ENDS	,F_NONE	,F_NONE	,F_NONE	,O_MACRO,SIZE_NONE},
	/* Non-useful */
	{INSTR68_OUTPUT,F_NONE	,F_NONE	,F_NONE	,O_MACRO,SIZE_NONE},
	{INSTR68_OPT	,F_NONE	,F_NONE	,F_NONE	,O_MACRO,SIZE_NONE},
	{INSTR68_SECTION,F_NONE	,F_NONE	,F_NONE	,O_MACRO,SIZE_NONE},

	{INSTR68_UNKNOWN}
};

/******** special cases ************
** OUTPUT *func(OUTPUT *,INPUT *);
** returns adjusted OUPUT if any changes has been applied
** otherwise NULL
*/


/* movea amongs others does not touch flags */
static OUTPUT *exprAddr(OUTPUT *exp,INSTR68 *instr)
{
	if((instr->op1.addrMode==AM_REG &&
			instr->op1.reg>=REG_A0 &&
			instr->op1.reg<=REG_A7) ||
		(instr->op2.addrMode==AM_REG &&
			instr->op2.reg>=REG_A0 &&
			instr->op2.reg<=REG_A7)) {
		exp->flagsSet=0;
		exp->flagsUsed=0;
		exp->flagsPreserved=F_ALL;
		return exp;
	}
	return NULL;
}

/* Some instructions touches CCR
** these are also assumed to use the whole CCR
*/
static OUTPUT *exprCCR(OUTPUT *exp,INSTR68 *instr)
{
	if(instr->op2.addrMode==AM_REG &&
			instr->op2.reg==REG_CCR) {
		exp->flagsSet=F_ALL;
		exp->flagsUsed=F_ALL;
		exp->flagsPreserved=0;
		return exp;
	}
	if(instr->op1.addrMode==AM_REG &&
			instr->op1.reg==REG_CCR) {
		exp->flagsSet=0;
		exp->flagsUsed=F_ALL;
		exp->flagsPreserved=F_ALL;
		return exp;
	}
	return NULL;
}


/* Some instructions exists both in A and CCR variants... */
static OUTPUT *exprAddrCCR(OUTPUT *exp,INSTR68 *instr)
{
	if(exprAddr(exp,instr))
		return exprAddr(exp,instr);
	return exprCCR(exp,instr);
}

/*************************************************/

static EXP68TAB *lookup(INSTRCODE68 instr)
{
	EXP68TAB *p;

	for(p=exp68tab;p->instr!=instr;p++)
		if(p->instr==INSTR68_UNKNOWN)
			return NULL;
	return p;
}


/* Error functions */

static void error(INPUT *in,E68_ERRORCODE err)
{
	OUTPUT ret;
	ret.type=IS_EXP68_ERROR;
	ret.lineNumber=in->lineNumber;
	ret.data.error.error=err;
	output(&ret);
}

static void errorNArgs(INPUT *in,OPSMODE mode,int nargs)
{
	OUTPUT ret;
	ret.type=IS_EXP68_ERROR;
	ret.lineNumber=in->lineNumber;
	ret.data.error.error=E68_E_NARGS;
	ret.data.error.opsMode=mode;
	ret.data.error.nargs=nargs;
	output(&ret);
}

static void errorOpsMode(INPUT *in,OPSMODE mode)
{
	OUTPUT ret;
	ret.type=IS_EXP68_ERROR;
	ret.lineNumber=in->lineNumber;
	ret.data.error.error=E68_E_OPSMODE;
	ret.data.error.opsMode=mode;
	output(&ret);
}

static void errorSize(INPUT *in)
{
	static OUTPUT ret;
	ret.type=IS_EXP68_ERROR;
	ret.lineNumber=in->lineNumber;
	ret.data.error.error=E68_E_SIZE;
	output(&ret);
}

/* 
** fill in operand sizes and uses
*/
static void fixOperands(INPUT *in,OUTPUT *exp)
{
	INSTR68 *instr;
	OPERAND *op1,*op2;
	int nop=0;
	SIZE size;
	instr=&exp->data.instr;
	op1=&instr->op1;
	op2=&instr->op2;
	if(op1->addrMode!=AM_NO_OPERAND)
		nop=1;
	if(op2->addrMode!=AM_NO_OPERAND)
		nop=2;

	size=instr->size;
	op1->size=size;
	op2->size=size;

	switch(instr->opsMode) {
	case O_NONE:
		if(nop!=0)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_NO_OPERAND;
		op2->mode=OP_NO_OPERAND;
		break;
	case O_NORMAL:
		if(nop!=2)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_READ;
		op2->mode=OP_WRITE;
		break;
	case O_SRC:
		if(nop!=1)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_READ;
		op2->mode=OP_NO_OPERAND;
		break;
	case O_DST:
		if(nop!=1)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_WRITE;
		op2->mode=OP_NO_OPERAND;
		break;
	case O_MOD:
		if(nop!=1)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_MODIFY;
		op2->mode=OP_NO_OPERAND;
		break;
	case O_MOD2:
		if(nop!=2)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_MODIFY;
		op2->mode=OP_MODIFY;
		break;
	case O_MDST:
		if(nop!=2)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_READ;
		op2->mode=OP_MODIFY;
		break;
	case O_MDST1:
		switch(nop) {
		case 1:
			op1->mode=OP_MODIFY;
			op2->mode=OP_NO_OPERAND;
			instr->opsMode=O_MOD;
			break;
		case 2:
			op1->mode=OP_READ;
			op2->mode=OP_MODIFY;
			instr->opsMode=O_MDST;
			break;
		default:
			errorNArgs(in,instr->opsMode,nop);
			break;
		}
		break;
	case O_FLOW:
		if(nop!=1)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_FLOW;
		op1->size=SIZE_L;
		op2->mode=OP_NO_OPERAND;
		break;
	case O_FLOW2: /* DBcc instruktionerna */
		if(nop!=2)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_MODIFY;
		if(op1->size<SIZE_B)
			op1->size=SIZE_W;
		op2->mode=OP_FLOW;
		op2->size=SIZE_L;
		break;
	case O_CHECK:
		if(nop!=2)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_READ;
		op2->mode=OP_READ;
		break;
	case O_DATA:
		if(nop!=1)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_DATA;
		op1->mode=OP_NO_OPERAND;
		break;
	case O_MACRO:
		if(nop>1)
			errorNArgs(in,instr->opsMode,nop);
		if(nop>=1)
			op1->mode=OP_MACRO;
		else
			op1->mode=OP_NO_OPERAND;
		break;
	case O_SPECIAL:
		if(nop!=1)
			errorNArgs(in,instr->opsMode,nop);
		op1->mode=OP_SPECIAL;
		op2->mode=OP_NO_OPERAND;
		break;
	default:
		errorOpsMode(in,instr->opsMode);
		break;
	}
}

/* This is the "main loop" for this step
*/
static void doSomeMore(void)
{
	static OUTPUT exp;
	EXP68TAB *tab;
	INPUT *in;

	/* Get next element to process */
	in=READER();

	/* EOF? */
	if(!in)
		return;

	/* copy the element */
	*(INPUT *)&exp=*in;

	/* fixup flags */
	switch(in->type) {
	case IS_INSTR:
		tab=lookup(in->data.instr.instr);
		if(!tab) {
			error(in,E68_E_INSTR);
			exp.flagsSet=F_NONE;
			exp.flagsUsed=F_NONE;
			exp.flagsPreserved=F_NONE;
			exp.data.instr.opsMode=O_SPECIAL;
			break;
		}
		exp.flagsSet=tab->flagsSet;
		exp.flagsUsed=tab->flagsUsed;
		exp.flagsPreserved=tab->flagsPreserved;
		exp.data.instr.opsMode=tab->opsMode;
		if(tab->realInstr)
			exp.data.instr.instr=tab->realInstr;
		if(!exp.data.instr.size) {
			exp.data.instr.size=tab->defSize;
			if(!tab->defSize) {
				if(tab->opsMode!=O_NONE && tab->opsMode!=O_FLOW)
					errorSize(in);
				exp.data.instr.size=SIZE_NONE;
			}
		}
		if(tab->funk)
			(EXPFUNK *)(tab->funk)(&exp,&exp.data.instr);
		fixOperands(in,&exp);
		break;
	case IS_LABEL:
		exp.flagsSet=0;
		exp.flagsUsed=0;
		exp.flagsPreserved=F_LABEL;
		break;
	default:
		exp.flagsSet=0;
		exp.flagsUsed=0;
		exp.flagsPreserved=F_ALL;
		break;
	}

	output(&exp);
}

/* Give output to the next step */
OUTPUT *exp68000(void)
{
	/* generate more if needed */
	if(curritem>=nitems) {
		resetOutput();
		doSomeMore();
	}

	/* EOF */
	if(curritem>=nitems)
		return NULL;

	/* return the next element */
	return &out[curritem++];
}
