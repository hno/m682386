/* Fixa opmoden, s� att de klaras av intel processorn */


/* Funktion:
**  1. L�s in instruktion
**  2. pre op 1
**  3. get op 1
**  4. post op 1
**  5. pre op 2
**  6. instruktion
**  7. post op 2
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "opmodes.h"

#define MAX_OUTPUT 20

/* Varifr�n f�r vi v�r input? */
#include "memind.h"
#define READER memIndirect
typedef MEMIND INPUT;

/* Vad har vi f�r output? */
typedef OPMODES OUTPUT;

/* Output k� */
static OUTPUT out[MAX_OUTPUT];
static int nitems=0;
static int curritem=0;

static void output(OUTPUT *);

/* L�gg till ett element i ouput k�n */
static void output(OUTPUT *elem)
{
	if(nitems>=MAX_OUTPUT) {
		fprintf(stderr,"MATCH86: Output que full in line %d\n",elem->lineNumber);
		exit(1);
	}
	out[nitems++]=*elem;
}

/* �terst�ll output k�n */
static void resetOutput(void)
{
	/* lite felkoll skadar aldrig... */
	if(curritem<nitems) {
		fprintf(stderr,"MATCH86: resetOutput on nonempty output queue!!! lastline:%d\n"
			,out[nitems-1].lineNumber);
		exit(1);
	}
	/* Ok.. t�m output k�n */
	curritem=0;
	nitems=0;
}


/* genSideInstr
** generera en sido instruktion,
** dvs en instruktion som handhar funktioner kring
** sj�lva instruktionen.
*/
static void genSideInstr(INPUT *in,INSTRCODE68 instr,OPERAND op1,OPERAND op2)
{
	OUTPUT out;
	memset(&out,0,sizeof(out));
	out.lineNumber=in->lineNumber;
	out.type=IS_INSTR;
	out.data.instr.instr=instr;
	out.data.instr.opsMode=O_SPECIAL;
	out.data.instr.op1=op1;
	out.data.instr.op2=op2;
	out.flagsSet=F_NONE;
	out.flagsUsed=F_NONE;
	out.flagsPreserved=F_ALL;
	output(&out);
}

/* genAdd
** generera en additions instruktion f�r
** att addera/subtrahera ifr�n ett adress register
*/
static void genAdd(INPUT *in,REGISTER reg,SIZE size)
{
	OPERAND op1,op2;
	memset(&op1,0,sizeof(op1));
	memset(&op2,0,sizeof(op1));
	op1.addrMode=AM_INDIRECT;
	op1.reg=reg;
	op1.index=REG_NONE;
	/* Fixa till ifall a7 adresseras med felaktig storlek */
	if(reg==REG_A7) {
		switch(size) {
		case 1:
			size=2;
			break;
		case -1:
			size=-2;
			break;
		default:
			break;
		}
	}
	sprintf(op1.constant.text,"%d",size);
	op1.size=SIZE_L;
	op1.mode=OP_READ;
	op2.addrMode=AM_REG;
	op2.reg=reg;
	op2.size=SIZE_L;
	op2.mode=OP_MODIFY;
	genSideInstr(in,INSTR68_LEA,op1,op2);
}

/* fix pre dec */
static void fixPre(INPUT *in,OPERAND op)
{
	/* �r det verkligen predec? */
	if(op.addrMode!=AM_INDIRECT_PREDEC) return;

	/* Det �r predec, r�kna ned registret med size */
	genAdd(in,op.reg,-op.size);
}

/* fix pre dec */
static void fixPost(INPUT *in,OPERAND op)
{
	/* �r det verkligen predec? */
	if(op.addrMode!=AM_INDIRECT_POSTINC) return;

	/* Det �r postinc, r�kna upp registret med size */
	genAdd(in,op.reg,op.size);
}

/* fixa att index register �r 32 bit sign extend 16 bit... */
static OPERAND fixIndex(INPUT *in,OPERAND op)
{
	OPERAND reg,temp;

	switch(op.addrMode) {
	case AM_INDIRECT:
	case AM_INDIRECT_PREDEC:
	case AM_INDIRECT_POSTINC:
		if(op.index!=REG_NONE && op.indexSize!=SIZE_L) {
			memset(&reg,0,sizeof(reg));
			reg.addrMode=AM_REG;
			reg.reg=op.index;
			reg.index=REG_NONE;
			reg.size=op.indexSize;
			if(reg.size<SIZE_B)
				reg.size=SIZE_W;
			reg.mode=OP_READ;
			temp=reg;
			temp.size=SIZE_L;
			temp.reg=REG_TEMP_INDEX;
			temp.mode=OP_WRITE;
			genSideInstr(in,INSTR68_EXT,reg,temp);
			op.index=REG_TEMP_INDEX;
			op.indexSize=SIZE_L;
			break;
		}
		break;
	default:
		break;
	}
	return op;
}


/* cleanOp
** rensa upp en operand
*/
static OPERAND cleanOp(OPERAND op)
{
	switch(op.addrMode) {
	case AM_INDIRECT_POSTINC:
	case AM_INDIRECT_PREDEC:
		op.addrMode=AM_INDIRECT;
		op.index=REG_NONE;
		break;
	default:
		break;
	}
	return op;
}

/* get op
** h�mta en operand, om operanden ej kan anv�ndas till minne, s� flytta in
** i tempor�rregister
** giltiga operander:
**   IMMEDIATE
**   REG
**	  Ensam operand, ej INDIRECT POSTINC
**	  Destinations operand register
*/
static OPERAND getOp(INPUT *in,OPERAND op)
{
	OPERAND temp;
	OPERAND nop1;

	switch(op.addrMode) {
	/* Operander som �r direkt anv�ndbara */
	case AM_NO_OPERAND:
	case AM_REG:
	case AM_IMMEDIATE:
		return op;
	/* Operander som inte �r anv�ndbara */
	case AM_INDIRECT_POSTINC:
		break;
	/* D�r anv�ndbarheten best�ms av detstinationen */
	default:
		switch(in->data.instr.op2.addrMode) {
		case AM_NO_OPERAND:
		case AM_REG:
			return cleanOp(op);
		default:
			break;
		}
		break;
	}
	/* �vrigt d�r operanden �r direkt anv�ndbar: */
	switch(op.mode) {
	case OP_FLOW:
		return op;
	default:
		break;
	}

	/* Nehepp.. m�ste g� via tempor�rregister */

	/* Hmm.. endast operander av typen READ �r till�tna till tempor�r */
	if(op.mode != OP_READ) {
		fprintf(stderr,"OPMODES: Invalid operand type for temporary reg in line %d\n",in->lineNumber);
		exit(1);
	}

	/* Flytta in operanden i tempor�r registret */

	/* Fixa till operanderna till flytten */
	nop1=cleanOp(op);
	memset(&temp,0,sizeof(temp));
	temp.addrMode=AM_REG;
	temp.reg=REG_TEMP;
	temp.index=REG_NONE;
	temp.mode=OP_WRITE;
	temp.size=op.size;

	/* Dags att flytta */
	genSideInstr(in,INSTR68_MOVE,nop1,temp);

	/* Fixa till retur v�rdet */
	temp.mode=OP_READ;

	/* Det var allt f�r idag */
	return temp;
}

/* genInstr
** generera sj�lva instruktionen
*/
static void genInstr(INPUT *in,OPERAND op1,OPERAND op2)
{
	OUTPUT out;
	*(INPUT *)&out=*in;
	out.data.instr.op1=op1;
	out.data.instr.op2=op2;
	output(&out);
}

/* Fixa en PC relativ operand
*/
static OPERAND fixPC(OPERAND op)
{
	switch(op.addrMode) {
	case AM_INDIRECT:
		if(op.reg==REG_PC) {
			op.reg=REG_NONE;
			if(!op.index)
				op.addrMode=AM_ABS_MEM;
		}
		break;
	case AM_MEM_INDIRECT:
		if(op.reg==REG_PC)
			op.reg=REG_NONE;
		break;
	default:
   	break;
	}
	return op;
}

/********************************************/
/* fixa diverse opmoden
** postinc och predec
** minne,minne
*/
static void doOpModes(void)
{
	INPUT *in;
	OPERAND op1,op2,rop1,rop2;
	/* 1. l�s instruktion */
	in=READER();
	if(!in)
		return;

	switch(in->type) {
	case IS_INSTR:
		/* Eleminera PC relativt om m�jligt */
		in->data.instr.op1=fixPC(in->data.instr.op1);
		in->data.instr.op2=fixPC(in->data.instr.op2);
		op1=in->data.instr.op1;
		op2=in->data.instr.op2;
		/* 2. pre op 1 */
		fixPre(in,op1);
		/* 3. get op 1 */
		op1=fixIndex(in,op1);
		rop1=getOp(in,op1);
		/* 4. post op 1 */
		fixPost(in,op1);
		/* 5. pre op 2 */
		fixPre(in,op2);
		/* 6. instruktion */
		rop2=cleanOp(op2);
		rop2=fixIndex(in,rop2);
		genInstr(in,rop1,rop2);
		/* 7. post op 2 */
		fixPost(in,op2);
		break;
	default:
		output(in);
		break;
	}
}


/* Returnera n�sta element i k�n, generera mera om det beh�vs */
OUTPUT *opModes(void)
{
	if(curritem>=nitems) {
		resetOutput();
		doOpModes();
	}
	if(curritem<nitems)
		return &out[curritem++];
	else
		return NULL;
}
