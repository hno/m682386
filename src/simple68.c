/* fîrenkla 68000 koden.
**
** Fîrenklingar:
**
**		movem.s	regs,-(a7)
**			push.s regs
**
**		movem.s	(a7)+,regs
**			pull.s regs
**
**		move.s	ea,-(a7)
**			push.s ea
**
**		move.s	(a7)+,ea
**			pull.s ea
**
**		pea <ea>
**			lea	temp.l,<ea>
**			push	temp.l
**
**		st  <ea>
**			mov	<ea>.b,255
**
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "info386.h"
#include "simple68.h"

#define MAX_OUTPUT 20

/* VarifrÜn fÜr vi vÜr input? */
#include "dotlabel.h"
#define READER dotLabel
typedef DOTLABEL INPUT;
typedef SIMPLE68 OUTPUT;

/* Output kî */
static OUTPUT out[MAX_OUTPUT];
static int nitems=0;
static int curritem=0;

static void output(OUTPUT *);

/* LÑgg till ett element i ouput kîn */
static void output(OUTPUT *elem)
{
	if(nitems>=MAX_OUTPUT) {
		fprintf(stderr,"SIMPLE68: Output que full in line %d\n",elem->lineNumber);
		exit(1);
	}
	out[nitems++]=*elem;
}

/* èterstÑll output kîn */
static void resetOutput(void)
{
	/* lite felkoll skadar aldrig... */
	if(curritem<nitems) {
		fprintf(stderr,"SIMPLE68: resetOutput on nonempty output queue!!! lastline:%d\n"
			,out[nitems-1].lineNumber);
		exit(1);
	}
	/* Ok.. tîm output kîn */
	curritem=0;
	nitems=0;
}

/* Generea en tom operand */
static OPERAND noOperand(void)
{
	OPERAND op;
	memset(&op,0,sizeof(op));
	op.addrMode=AM_NO_OPERAND;
	op.reg=REG_NONE;
	op.index=REG_NONE;
	return op;
}

/* Modifiera en operand med size och mode */
static OPERAND stdOp(OPERAND op,SIZE size,OPMODE mode)
{
	op.mode=mode;
	op.size=size;
	return op;
}

/* Fixa en temporÑrreg operand */
static OPERAND tempReg(SIZE size,OPMODE mode)
{
	OPERAND op;
	op=noOperand();
	op.addrMode=AM_REG;
	op.reg=REG_TEMP;
	op.index=REG_NONE;
	op.size=size;
	op.mode=mode;
	return op;
}

/* Fixa en immediate operand */
static OPERAND immediate(int value,SIZE size)
{
	OPERAND op;
	op=noOperand();
	op.addrMode=AM_IMMEDIATE;
	sprintf(op.constant.text,"%d",value);
	op.size=size;
	op.mode=OP_READ;
	return op;
}
/* Generera en ut instruktion */
static void outputInstr(INPUT *in,INSTRCODE68 instr,OPERAND op1,OPERAND op2,OPSMODE mode)
{
	OUTPUT out;
	*(INPUT *)&out=*in;
	out.data.instr.instr=instr;
	out.data.instr.op1=op1;
	out.data.instr.op2=op2;
	out.data.instr.opsMode=mode;
	output(&out);
}

/* push/pull */
static int doPushPull(INPUT *in,OPERAND op1,OPERAND op2)
{
	/* PUSH */
	if(op2.addrMode==AM_INDIRECT_PREDEC && op2.reg==REG_SP) {
		outputInstr(in,INSTR68_PUSH,op1,noOperand(),O_SRC);
		return 1;
	}

	/* PULL */
	if(op1.addrMode==AM_INDIRECT_POSTINC && op1.reg==REG_SP) {
		outputInstr(in,INSTR68_PULL,op2,noOperand(),O_DST);
		return 1;
	}

	/* nehepp, det var inte en push/pull */
	return 0;
}

/* Greja lite mera... */
static void doSomeMore(void)
{
	INPUT *in;
	OPERAND op1;
	OPERAND op2;

	in=READER();
	if(!in) return;
	switch(in->type) {
	case IS_INSTR:
		op1=in->data.instr.op1;
		op2=in->data.instr.op2;
		switch(in->data.instr.instr) {
		case INSTR68_PEA:
			outputInstr(in,INSTR68_LEA,op1,tempReg(SIZE_L,OP_WRITE),O_NORMAL);
			outputInstr(in,INSTR68_PUSH,tempReg(SIZE_L,OP_READ),noOperand(),O_SRC);
			break;
		case INSTR68_ST:
			outputInstr(in,INSTR68_MOVE,immediate(255,SIZE_B),stdOp(op1,SIZE_B,OP_WRITE),O_NORMAL);
			break;
		case INSTR68_MOVEM:
			if(!doPushPull(in,op1,op2))
				goto normal;
			break;
		case INSTR68_MOVE:
			if(!doPushPull(in,op1,op2))
				goto normal;
			break;
		default:
normal:
			output((OUTPUT *)in);
			break;
		}
		break;
	default:
		output((OUTPUT *)in);
		break;
	}
}

/* Returnera nÑsta element i kîn, generera mera om det behîvs */
OUTPUT *simple68(void)
{
	if(curritem>=nitems) {
		resetOutput();
		doSomeMore();
	}
	if(curritem<nitems)
		return &out[curritem++];
	else
		return NULL;
}
