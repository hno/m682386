/* Fixa multireg
** ordning i minnet:
**    d0->a7 (d0 lÑgst minnesadress, a7 hîgst)
**	dvs vid -(An) fîrst A7->D0
** îvriga fall fîrst D0
** Vid addresseringar mot minne (ej postinc/predec) mÜste konstanten
** îkas pÜ med storleken
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "info386.h"
#include "multireg.h"

#define MAX_OUTPUT 20

/* VarifrÜn fÜr vi vÜr input? */
#include "simple68.h"
#define READER simple68
typedef SIMPLE68 INPUT;
typedef MULTIREG OUTPUT;

/* Output kî */
static OUTPUT out[MAX_OUTPUT];
static int nitems=0;
static int curritem=0;

static void output(OUTPUT *);

/* LÑgg till ett element i ouput kîn */
static void output(OUTPUT *elem)
{
	if(nitems>=MAX_OUTPUT) {
		fprintf(stderr,"MULTIREG: Output que full in line %d\n",elem->lineNumber);
		exit(1);
	}
	out[nitems++]=*elem;
}

/* èterstÑll output kîn */
static void resetOutput(void)
{
	/* lite felkoll skadar aldrig... */
	if(curritem<nitems) {
		fprintf(stderr,"MULTIREG: resetOutput on nonempty output queue!!! lastline:%d\n"
			,out[nitems-1].lineNumber);
		exit(1);
	}
	/* Ok.. tîm output kîn */
	curritem=0;
	nitems=0;
}

/* Generera en ut instruktion */
static void outputInstr(INPUT *in,OPERAND op1,OPERAND op2)
{
	OUTPUT out;
	*(INPUT *)&out=*in;
	out.data.instr.op1=op1;
	out.data.instr.op2=op2;
	output(&out);
}

/* Fixa till en operand med ev offset */
static OPERAND fixOperand(OPERAND op,int offset)
{
	if(strlen(op.constant.text)) {
		CONSTANT temp=op.constant;
		sprintf(op.constant.text,"%s+%d",temp.text,offset);
	} else sprintf(op.constant.text,"%d",offset);
	return op;
}

/* gîr en lea av adressen */
static OPERAND doLea(INPUT *in,OPERAND op)
{
	OUTPUT out;
	OPERAND leaop,addrop;

	/* Ordna till en REG_TEMP operand */
	memset(&leaop,0,sizeof(leaop));
	leaop.addrMode=AM_REG;
	leaop.reg=REG_TEMP;
	leaop.mode=OP_WRITE;
	leaop.size=SIZE_L;

	/* Ordna adress operanden */
	addrop=op;
	addrop.size=SIZE_NONE;
	addrop.mode=OP_READ;
	/* POSTINC och PREDEC mÜste reduceras till INDIRECT */
	switch(addrop.addrMode) {
	case AM_INDIRECT_PREDEC:
	case AM_INDIRECT_POSTINC:
		addrop.addrMode=AM_INDIRECT;
		addrop.index=REG_NONE;
		strcpy(addrop.constant.text,"");
		break;
	case AM_NO_OPERAND:
		/* PUSH, PULL har ingen adress operand */
		memset(&leaop,0,sizeof(leaop));
		return leaop;
	default:
		break;
	}

	/* Gîr en LEA REG_TEMP,op */
	*(INPUT *)&out=*in;
	out.data.instr.instr=INSTR68_LEA;
	out.data.instr.opsMode=O_NORMAL;
	out.data.instr.size=SIZE_NONE;
	out.data.instr.op1=addrop;
	out.data.instr.op2=leaop;
	output(&out);

	/* Ordna till leaop sÜ att det Ñr en indirect mot REG_TEMP */
	leaop.addrMode=AM_INDIRECT;
	leaop.mode=op.mode;
	leaop.size=op.size;

	return leaop;
}

/* Gôr en updatering av postinc / predec registret */
static void doUpdate(INPUT *in,OPERAND op,OPERAND leaop,int offset)
{
	OUTPUT out;

	switch(op.addrMode) {
	case AM_INDIRECT_PREDEC:
	case AM_INDIRECT_POSTINC:
		/* Den mÜste updateras */

		/* Ordna till postinc/predec till register operand */
		op.addrMode=AM_REG;
		op.mode=OP_WRITE;
		op.size=SIZE_L;

		/* Ordna till adress operanden */
		leaop=fixOperand(leaop,offset);
		leaop.mode=OP_READ;
		leaop.size=SIZE_NONE;


		*(INPUT *)&out=*in;
		out.data.instr.instr=INSTR68_LEA;
		out.data.instr.opsMode=O_NORMAL;
		out.data.instr.size=SIZE_NONE;
		out.data.instr.op1=leaop;
		out.data.instr.op2=op;
		output(&out);
		break;
	default:
		break;
	}
}

static void doForvardMultiRegOp1(INPUT *in)
{
	OPERAND inop,op,regop,leaop;
	REGMASK mask;
	REGISTER reg;
	int offset;

	mask=in->data.instr.op1.regs.mask;
	inop=in->data.instr.op2;
	regop=in->data.instr.op1;
	regop.addrMode=AM_REG;
	offset=0;

	leaop=doLea(in,inop);

	for(offset=0,reg=REG_D0;reg<=REG_A7;reg++) {
		if(REGMASKSET(mask,reg)) {
			regop.reg=reg;
			op=fixOperand(leaop,offset);
			outputInstr(in,regop,op);
			offset+=in->data.instr.size;
		}
	}

	doUpdate(in,inop,leaop,offset);
}

/* MOVEM.s regs,-(An), PUSH regs */
static void doReverseMultiRegOp1(INPUT *in)
{
	OPERAND inop,op,regop,leaop;
	REGMASK mask;
	REGISTER reg;
	int offset;

	mask=in->data.instr.op1.regs.mask;
	inop=in->data.instr.op2;
	regop=in->data.instr.op1;
	regop.addrMode=AM_REG;
	offset=0;

	leaop=doLea(in,inop);

	for(offset=0,reg=REG_A7;reg>=REG_D0;reg--) {
		if(REGMASKSET(mask,reg)) {
			regop.reg=reg;
			offset-=in->data.instr.size;
			op=fixOperand(leaop,offset);
			outputInstr(in,regop,op);
		}
	}

	doUpdate(in,inop,leaop,offset);
}

static void doForvardMultiRegOp2(INPUT *in)
{
	OPERAND inop,op,regop,leaop;
	REGMASK mask;
	REGISTER reg;
	int offset;

	mask=in->data.instr.op2.regs.mask;
	inop=in->data.instr.op1;
	regop=in->data.instr.op2;
	regop.addrMode=AM_REG;
	offset=0;

	leaop=doLea(in,inop);

	for(offset=0,reg=REG_D0;reg<=REG_A7;reg++) {
		if(REGMASKSET(mask,reg)) {
			regop.reg=reg;
			op=fixOperand(leaop,offset);
			outputInstr(in,op,regop);
			offset+=in->data.instr.size;
		}
	}
	doUpdate(in,inop,leaop,offset);
}

static void doMultiReg(INPUT *in)
{
	/* Multireg op 1 */
	if(in->data.instr.op1.addrMode==AM_MULTI_REG) {
		if(in->data.instr.op2.addrMode==AM_INDIRECT_PREDEC
			|| in->data.instr.instr==INSTR68_PUSH)
			doReverseMultiRegOp1(in);
		else
			doForvardMultiRegOp1(in);
	}

	/* Multireg op 2 */
	if(in->data.instr.op2.addrMode==AM_MULTI_REG) {
		doForvardMultiRegOp2(in);
	}
}

/* Greja lite mera... */
static void doSomeMore(void)
{
	INPUT *in;

	in=READER();
	if(!in) return;
	switch(in->type) {
	case IS_INSTR:
		if(in->data.instr.op1.addrMode==AM_MULTI_REG ||
			in->data.instr.op2.addrMode==AM_MULTI_REG)
			doMultiReg(in);
		else
			output((OUTPUT *)in);
		break;
	default:
		output((OUTPUT *)in);
		break;
	}
}

/* Returnera nÑsta element i kîn, generera mera om det behîvs */
OUTPUT *multiReg(void)
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
