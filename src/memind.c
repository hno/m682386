#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "memind.h"

#define MAX_OUTPUT 10

/* Varifr†n f†r vi v†r input? */
#include "multireg.h"
#define READER multiReg
typedef MULTIREG INPUT;

/* Vad har vi f”r output? */
typedef MEMIND OUTPUT;

/* Output k” */
static OUTPUT out[MAX_OUTPUT];
static int nitems=0;
static int curritem=0;

void output(OUTPUT *);

/* L„gg till ett element i ouput k”n */
static void output(OUTPUT *elem)
{
	if(nitems>=MAX_OUTPUT) {
		fprintf(stderr,"MATCH86: Output que full in line %d\n",elem->lineNumber);
		exit(1);
	}
	out[nitems++]=*elem;
}

/* terst„ll output k”n */
static void resetOutput(void)
{
	/* lite felkoll skadar aldrig... */
	if(curritem<nitems) {
		fprintf(stderr,"MATCH86: resetOutput on nonempty output queue!!! lastline:%d\n"
			,out[nitems-1].lineNumber);
		exit(1);
	}
	/* Ok.. t”m output k”n */
	curritem=0;
	nitems=0;
}


/* genSideInstr
** generera en sido instruktion,
** dvs en instruktion som handhar funktioner kring
** sj„lva instruktionen.
*/
static OPERAND fixMemIndirect(INPUT *in,OPERAND op,int opnr)
{
	OUTPUT out;
	OPERAND nop;
	OPERAND mop;
	OPERAND rop;

	if(op.addrMode!=AM_MEM_INDIRECT)
		return op;

	memset(&out,0,sizeof(out));
	memset(&nop,0,sizeof(nop));

	/* Fixa memory adress operanden */
	mop=op;
	mop.addrMode=AM_INDIRECT;
	mop.size=SIZE_L;
	mop.mode=OP_READ;

	/* Fixa slut operanen */
	nop=op;
	nop.addrMode=AM_INDIRECT;
	switch(opnr) {
	case 1:
		nop.reg=REG_TEMP_MEMIND1;
		break;
	case 2:
		nop.reg=REG_TEMP_MEMIND2;
		break;
	}
	nop.index=nop.outerIndex;
	nop.constant=nop.outerDispl;

	/* Fixa register operanden */
	rop=nop;
	rop.addrMode=AM_REG;
	rop.index=REG_NONE;
	rop.size=SIZE_L;
	rop.mode=OP_WRITE;



	/* Flytta in minnesadressen i ett register */
	out.lineNumber=in->lineNumber;
	out.type=IS_INSTR;
	out.data.instr.instr=INSTR68_MOVE;
	out.data.instr.opsMode=O_NORMAL;
	out.data.instr.size=SIZE_L;
	out.data.instr.op1=mop;
	out.data.instr.op2=rop;
	out.flagsSet=F_NONE;
	out.flagsUsed=F_NONE;
	out.flagsPreserved=F_ALL;
	output(&out);

	/* returnera fixad operand */
	return nop;
}

/********************************************/
/* fixa memory indirect
*/
static void doSomeMore(void)
{
	INPUT *in;
	OPERAND op1,op2;
	OUTPUT out;

	/* 1. l„s instruktion */
	in=READER();
	if(!in)
		return;

	*(INPUT *)&out=*in;

	switch(in->type) {
	case IS_INSTR:
		op1=in->data.instr.op1;
		op2=in->data.instr.op2;
		op1=fixMemIndirect(in,op1,1);
		op2=fixMemIndirect(in,op2,2);
		out.data.instr.op1=op1;
		out.data.instr.op2=op2;
		break;
	default:
		break;
	}
	output(&out);
}


/* Returnera n„sta element i k”n, generera mera om det beh”vs */
OUTPUT *memIndirect(void)
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
