/* matcha 68000 mot 80386
** och v„nd operandordningen till INTEL
** obs! v„ndningen sker absolut sist, dvs i newInstr()
** s† hela modulen jobbar med Motorola modellen.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "info386.h"
#include "match86.h"

#define MAX_OUTPUT 20

/* Varifr†n f†r vi v†r input? */
#include "x_is_c.h"
#define READER XisC
typedef X_IS_C INPUT;
typedef MATCH86 OUTPUT;

/* Vad „r det f”r typ av instruktion som genereras?
** dvs hur skall den p†verka flaggor mm?
*/
typedef enum {
	I_NORMAL, /* En "normal" direkt”vers„ttning av instruktionen */
	I_PREINSTR,/* Denna instruktion utf”rs innan verklig instruktion */
	I_POSTINSTR,/* Denna instruktion utf”rs efter verklig instruktion */
	I_SPECIAL,	/* Jag vet nog vad jag sysslar med... (tror jag) */
	I_SPECIALFLAGS,/* Fixar flaggor, men alla operander „r OP_READ, pga att de egentligen inte f”r„ndras */
	I_FLAGS	/* Denna instruktion p†verkar flaggorna, men skiljer sig vad g„llande operanderna */
} INSTRMODE;

/* Hur ser v†r finfina tabell ut? */
typedef struct table TABLE;
typedef int SPECIAL(INPUT *,TABLE *);	/* Specialfalls hanterare */
struct table {
	INSTRCODE68	instr68;	/* den 68000 instruktion som skall ”vers„ttas */
	INSTRCODE86 instr86; /* motsvarande 386 instruktion */
	SPECIAL *special;		/* specialfalls hanterare */
};

/* Specialfall */
static SPECIAL move;
static SPECIAL dbf;
static SPECIAL dbcc;
static SPECIAL dc;
static SPECIAL dcbDs;
static SPECIAL setZtoNC;
static SPECIAL swap;
static SPECIAL ext,extb;
static SPECIAL shift;
static SPECIAL tst;
static SPECIAL jump;
static SPECIAL divFlags,mulFlags;
static SPECIAL include;
static SPECIAL rs;
static SPECIAL movem;
static SPECIAL push;
static SPECIAL pull;

/* ™vers„ttnings tabell */
static TABLE table[]={
	{INSTR68_ABCD,INSTR86_INT_ABCD},
	{INSTR68_ADD,INSTR86_ADD},
	{INSTR68_ADDX,INSTR86_ADC},
	{INSTR68_AND,INSTR86_AND},
	{INSTR68_ASL,INSTR86_SAL,shift},
	{INSTR68_ASR,INSTR86_SAR,shift},
	{INSTR68_BRA,INSTR86_JMP,jump},
	/* condition code jumps */
	{INSTR68_BHI,INSTR86_JA},
	{INSTR68_BLS,INSTR86_JBE},
	{INSTR68_BCC,INSTR86_JNC},
	{INSTR68_BCS,INSTR86_JC},
	{INSTR68_BNE,INSTR86_JNE},
	{INSTR68_BEQ,INSTR86_JE},
	{INSTR68_BVC,INSTR86_JNO},
	{INSTR68_BVS,INSTR86_JO},
	{INSTR68_BPL,INSTR86_JNS},
	{INSTR68_BMI,INSTR86_JS},
	{INSTR68_BGE,INSTR86_JGE},
	{INSTR68_BLT,INSTR86_JL},
	{INSTR68_BGT,INSTR86_JG},
	{INSTR68_BLE,INSTR86_JLE},
	/************************/
	{INSTR68_BCHG,INSTR86_BTC,setZtoNC},
	{INSTR68_BCLR,INSTR86_BTR,setZtoNC},
	{INSTR68_BFCHG},
	{INSTR68_BFCLR},
	{INSTR68_BFEXTS},
	{INSTR68_BFEXTU},
	{INSTR68_BFFFO},
	{INSTR68_BFINS},
	{INSTR68_BFSET},
	{INSTR68_BFTST},
	{INSTR68_BKPT},
	{INSTR68_BSET,INSTR86_BTS,setZtoNC},
	{INSTR68_BSR,INSTR86_CALL},
	{INSTR68_BTST,INSTR86_BT,setZtoNC},
	{INSTR68_CALLM},
	{INSTR68_CAS},
	{INSTR68_CAS2},
	{INSTR68_CHK},
	{INSTR68_CHK2},
	{INSTR68_CLR,INSTR86_CLR},
	{INSTR68_CMP,INSTR86_CMP},
	{INSTR68_CMP2},
	/*condition code dec&branch*/
	{INSTR68_DBT},
	{INSTR68_DBF,INSTR86_UNKNOWN,dbf},
	{INSTR68_DBHI,INSTR86_JA,dbcc},
	{INSTR68_DBLS,INSTR86_JBE,dbcc},
	{INSTR68_DBCC,INSTR86_JNC,dbcc},
	{INSTR68_DBCS,INSTR86_JC,dbcc},
	{INSTR68_DBNE,INSTR86_JNE,dbcc},
	{INSTR68_DBEQ,INSTR86_JE,dbcc},
	{INSTR68_DBVC,INSTR86_JNO,dbcc},
	{INSTR68_DBVS,INSTR86_JO,dbcc},
	{INSTR68_DBPL,INSTR86_JNS,dbcc},
	{INSTR68_DBMI,INSTR86_JS,dbcc},
	{INSTR68_DBGE,INSTR86_JGE,dbcc},
	{INSTR68_DBLT,INSTR86_JL,dbcc},
	{INSTR68_DBGT,INSTR86_JG,dbcc},
	{INSTR68_DBLE,INSTR86_JLE,dbcc},
	/**************************/
	{INSTR68_DIVS,INSTR86_IDIV,divFlags},
	{INSTR68_DIVSL},
	{INSTR68_DIVU,INSTR86_DIV,divFlags},
	{INSTR68_DIVUL},
	{INSTR68_EOR,INSTR86_XOR},
	{INSTR68_EXG,INSTR86_XCHG},
	{INSTR68_EXT,INSTR86_MOVSX,ext},
	{INSTR68_EXTB,INSTR86_MOVSX,extb},
	{INSTR68_ILLEGAL},
	{INSTR68_JMP,INSTR86_JMP,jump},
	{INSTR68_JSR,INSTR86_CALL,jump},
	{INSTR68_LEA,INSTR86_LEA},
	{INSTR68_LINK},
	{INSTR68_LSL,INSTR86_SHL,shift},
	{INSTR68_LSR,INSTR86_SHR,shift},
	{INSTR68_MOVE,INSTR86_MOV,move},
	{INSTR68_MOVEC},
	{INSTR68_MOVEM,INSTR86_MOV,movem},
	{INSTR68_MOVEP},
	{INSTR68_MOVES},
	{INSTR68_MULS,INSTR86_IMUL,mulFlags},
	{INSTR68_MULSL},
	{INSTR68_MULU,INSTR86_MUL,mulFlags},
	{INSTR68_MULUL},
	{INSTR68_NBCD},
	{INSTR68_NEG,INSTR86_NEG},
	{INSTR68_NEGX},
	{INSTR68_NOP,INSTR86_NOP},
	{INSTR68_NOT,INSTR86_NOT},
	{INSTR68_OR,INSTR86_OR},
	{INSTR68_PACK},
	{INSTR68_PEA},
	{INSTR68_RESET},
	{INSTR68_ROL,INSTR86_ROL,shift},
	{INSTR68_ROXL,INSTR86_RCL,shift},
	{INSTR68_ROR,INSTR86_ROR,shift},
	{INSTR68_ROXR,INSTR86_RCR,shift},
	{INSTR68_RTD},
	{INSTR68_RTE},
	{INSTR68_RTM},
	{INSTR68_RTR},
	{INSTR68_RTS,INSTR86_RET},
	{INSTR68_SBCD,INSTR86_INT_SBCD},
	{INSTR68_ST},
	{INSTR68_SF},
	{INSTR68_SHI,INSTR86_SETA},
	{INSTR68_SLS,INSTR86_SETLE},
	{INSTR68_SCC,INSTR86_SETNC},
	{INSTR68_SCS,INSTR86_SETC},
	{INSTR68_SNE,INSTR86_SETNE},
	{INSTR68_SEQ,INSTR86_SETE},
	{INSTR68_SVC,INSTR86_SETNO},
	{INSTR68_SVS,INSTR86_SETO},
	{INSTR68_SPL,INSTR86_SETNS},
	{INSTR68_SMI,INSTR86_SETS},
	{INSTR68_SGE,INSTR86_SETGE},
	{INSTR68_SLT,INSTR86_SETL},
	{INSTR68_SGT,INSTR86_SETG},
	{INSTR68_SLE,INSTR86_SETLE},
	/*********************/
	{INSTR68_SUB,INSTR86_SUB},
	{INSTR68_SUBX,INSTR86_SBB},
	{INSTR68_SWAP,INSTR86_ROL,swap},
	{INSTR68_TAS},
	{INSTR68_TST,INSTR86_CMP,tst},
	{INSTR68_UNLK},
	{INSTR68_UNPK},
	/* devpac special */
	{INSTR68_PUSH,INSTR86_PUSH,push},
	{INSTR68_PULL,INSTR86_POP,pull},
	/* Makron */
	{INSTR68_CLR_X,INSTR86_CLC},
	{INSTR68_RTKF,INSTR86_RTKF},
	{INSTR68_MACRO_USE,INSTR86_MACRO_USE},
	{INSTR68_RS,INSTR86_UNKNOWN,rs},
	{INSTR68_RSSET,INSTR86_RSTRUCSET},
	{INSTR68_RSRESET,INSTR86_RSTRUCRESET},
	/* diverse direktiv */
	{INSTR68_DC,INSTR86_UNKNOWN,dc},
	{INSTR68_DCB,INSTR86_UNKNOWN,dcbDs},
	{INSTR68_DS,INSTR86_UNKNOWN,dcbDs},
	{INSTR68_INCBIN,INSTR86_INCLUDE,include},
	{INSTR68_EQU,INSTR86_EQU},
	{INSTR68_SET,INSTR86_SET},
	{INSTR68_OUTPUT},
	{INSTR68_OPT},
	{INSTR68_SECTION},
	{INSTR68_INCLUDE,INSTR86_INCLUDE,include},
	{INSTR68_IFNE,INSTR86_IF},
	{INSTR68_ENDC,INSTR86_ENDIF},
	{INSTR68_EVEN},
	{INSTR68_CNOP},
	{INSTR68_MACRO},
	{INSTR68_ENDM},
	{INSTR68_STRUCT},
	{INSTR68_ENDS},
	{INSTR68_REPT,INSTR86_REPT},
	{INSTR68_ENDR,INSTR86_ENDM},
	{INSTR68_UNKNOWN}
};

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

/* nagot gick snett... */
static void errorUnimpl(INPUT *in)
{
	OUTPUT new;
	memset(&new,0,sizeof(new));
	new.type=IS_MATCH86_ERROR;
	new.lineNumber=in->lineNumber;
	new.data.error.error=M86_E_UNIMPL;
	new.data.error.instr68=in->data.instr.instr;
	new.data.error.instrText=in->data.instr.instrText;
	output(&new);
}

static void errorInfo86(INPUT *in,INSTRCODE86 instr)
{
	OUTPUT new;
	memset(&new,0,sizeof(new));
	new.type=IS_MATCH86_ERROR;
	new.lineNumber=in->lineNumber;
	new.data.error.error=M86_E_INFO86;
	new.data.error.instr86=instr;
	new.data.error.instrText=in->data.instr.instrText;
	output(&new);
}

static void errorFlagsSet(INPUT *in,FLAGMASK flags)
{
	OUTPUT new;
	memset(&new,0,sizeof(new));
	new.type=IS_MATCH86_ERROR;
	new.lineNumber=in->lineNumber;
	new.data.error.error=M86_E_FLAGS_SET;
	new.data.error.flags=flags;
	output(&new);
}

static void errorFlagsUsed(INPUT *in,FLAGMASK flags)
{
	OUTPUT new;
	memset(&new,0,sizeof(new));
	new.type=IS_MATCH86_ERROR;
	new.lineNumber=in->lineNumber;
	new.data.error.error=M86_E_FLAGS_USED;
	new.data.error.flags=flags;
	output(&new);
}

static void errorFlagsPreserved(INPUT *in,FLAGMASK flags)
{
	OUTPUT new;
	memset(&new,0,sizeof(new));
	new.type=IS_MATCH86_ERROR;
	new.lineNumber=in->lineNumber;
	new.data.error.error=M86_E_FLAGS_PRESERVED;
	new.data.error.flags=flags;
	output(&new);
}

static void errorOpMode(INPUT *in)
{
	OUTPUT new;
	memset(&new,0,sizeof(new));
	new.type=IS_MATCH86_ERROR;
	new.lineNumber=in->lineNumber;
	new.data.error.error=M86_E_OPMODE;
	output(&new);
}

static void errorSize(INPUT *in,SIZE size)
{
	OUTPUT new;
	memset(&new,0,sizeof(new));
	new.type=IS_MATCH86_ERROR;
	new.lineNumber=in->lineNumber;
	new.data.error.error=M86_E_SIZE;
	new.data.error.size=size;
}

/* Tabell lookup */
static TABLE *lookup(INSTRCODE68 instr)
{
	TABLE *ptr;
	for(ptr=table;ptr->instr68!=instr && ptr->instr68!=INSTR68_UNKNOWN;ptr++);
	if(ptr->instr68!=INSTR68_UNKNOWN && ptr->instr86!=INSTR86_UNIMPL)
		return ptr;
	else
		return NULL;
}

/* newInstr
** Addera en ny instruktion till output
** kontrollera flagganv„ndning
** kontrollera operandanv„ndning
*/
static void newInstr(INPUT *in,INSTRCODE86 instr,OPERAND *op1,OPERAND *op2,INSTRMODE mode)
{
	OUTPUT new;
	INFO86 *info;
	INFO86 unknown;

	FLAGMASK set,used,preserved;

	/* T”m n”dv„ndniga strukturer */
	memset(&new,0,sizeof(new));
	new.data.instr.instrText=in->data.instr.instrText;
	new.data.instr.op1.reg=REG_NONE;
	new.data.instr.op1.index=REG_NONE;
	new.data.instr.op2=new.data.instr.op1;

	/* Leta reda p† information om instruktionen */
	info=info386(instr);
	if(!info || info->opsMode==OP_UNKNOWN) {
		errorInfo86(in,instr);
		memset(&unknown,0,sizeof(INFO86));
		info=&unknown;
		mode=I_SPECIAL;
	}

	/* Klura ut vilka/hur flaggor instruktionen MSTE jobba med */
	switch(mode) {
	case I_NORMAL:
		set=in->flagsSet;
		used=in->flagsUsed;
		preserved=in->flagsPreserved;
		if(in->data.instr.opsMode != info->opsMode &&
				in->data.instr.opsMode != O_SPECIAL)
			errorOpMode(in);
		break;
	case I_PREINSTR:
		set=0;
		used=0;
		preserved=in->flagsUsed|in->flagsPreserved;
		break;
	case I_POSTINSTR:
		set=0;
		used=0;
		preserved=in->flagsSet|in->flagsPreserved;
		break;
	case I_SPECIALFLAGS:
	case I_FLAGS:
		set=in->flagsSet;
		used=in->flagsUsed;
		preserved=in->flagsPreserved;
		break;
	case I_SPECIAL:
		set=0;
		used=0;
		preserved=0;
		break;
	}

	/* Fyll i ny struktur */
	new.type=IS_INSTR;
	new.lineNumber=in->lineNumber;
	new.data.instr.instr=instr;
	new.data.instr.size=in->data.instr.size;
	new.data.instr.opsMode=info->opsMode;
	if(op2 && op2->addrMode!=AM_NO_OPERAND) {
		new.data.instr.op1=*op2;
		new.data.instr.op2=*op1;
	}
	else if(op1 && op1->addrMode!=AM_NO_OPERAND) {
		new.data.instr.op1=*op1;
	}
	new.flagsSet=info->flagsSet & set;
	new.flagsUsed=info->flagsUsed & used;
	new.flagsPreserved=info->flagsPreserved & preserved;

	/* Hur „r det med flagganv„ndningen? */
	if(new.flagsSet  != set)
		errorFlagsSet(in,~new.flagsSet & set);
	if(new.flagsUsed != used)
		errorFlagsUsed(in,~new.flagsUsed & used);
	if(new.flagsPreserved != preserved)
		errorFlagsPreserved(in,~new.flagsPreserved & preserved);

	/* Hur „r det med operand anv„ndnigen */
	switch(mode) {
	case I_SPECIALFLAGS:
		new.data.instr.op1.mode=OP_READ;
		new.data.instr.op2.mode=OP_READ;
		break;
	default:
		break;
   }

	output(&new);
}

/* newLabel
** generera en label
*/
static void newLabel(INPUT *in,char *label)
{
	OUTPUT out;
	memset(&out,0,sizeof(out));
	out.type=IS_LABEL;
	out.lineNumber=in->lineNumber;
	out.flagsPreserved=F_ALL;
	strcpy(out.data.label.text,label);
	output(&out);
}

/* Generera en standard instruktion */
static void newStdInstr(INPUT *in,INSTRCODE86 instr)
{
	newInstr(in,instr,&in->data.instr.op1,&in->data.instr.op2,I_NORMAL);
}

/* tom operand */
static OPERAND cleanOperand(void)
{
	OPERAND ret;
	memset(&ret,0,sizeof(ret));
	ret.addrMode=AM_NO_OPERAND;
	ret.mode=OP_UNKNOWN;
	ret.reg=REG_NONE;
	ret.index=REG_NONE;
	return ret;
}

/* tempor„r register operand */
static OPERAND tmpReg(void)
{
	OPERAND tmp;
	tmp=cleanOperand();
	tmp.addrMode=AM_REG;
	tmp.reg=REG_TEMP;
	return tmp;
}

/********* Diverse Specialfall ****************
** som specialfall r„knas instruktioner som inte
** har en enkel 1-1 ”vers„ttning, utan ”vers„tts
** till mer „n 1 instruktion, eller olika instrukioner
** beroende p† operander och flaggp†verkning
*/

/* move
** det beh”ver g”ras en or f”r att f† till flagginfo
*/
static int move(INPUT *in,TABLE *info)
{
	OPERAND *op1,*op2,tmp;
	/* P†verkas n†gra flaggor? om inte inget specialfall. */
	if(!in->flagsSet) return 0;

	op1=&in->data.instr.op1;
	op2=&in->data.instr.op2;
	tmp=tmpReg();
	tmp.size=in->data.instr.size;
	tmp.mode=OP_READ;

	/* De tre olika m”jligheterna: */
	/* K„llan „r ett register */
	if(op1->addrMode==AM_REG) {
		newInstr(in,INSTR86_OR,op1,op1,I_SPECIALFLAGS);
		newInstr(in,INSTR86_MOV,op1,op2,I_POSTINSTR);
	}
	/* Destinationen „r ett register */
	else if(op2->addrMode==AM_REG) {
		newInstr(in,INSTR86_MOV,op1,op2,I_PREINSTR);
		newInstr(in,INSTR86_OR,op2,op2,I_SPECIALFLAGS);
	}
	/* varken k„lla eller destination „r register */
	else {
		tmp.mode=OP_WRITE;
		newInstr(in,INSTR86_MOV,op1,&tmp,I_PREINSTR);
		tmp.mode=OP_READ;
		newInstr(in,INSTR86_OR,&tmp,&tmp,I_SPECIALFLAGS);
		tmp.mode=OP_READ;
		newInstr(in,INSTR86_MOV,&tmp,op2,I_POSTINSTR);
	}
	return 1;
}

/* dbf
** ers„tts med
**   SUB reg,1
**   JNC label
*/
static int dbf(INPUT *in,TABLE *info)
{
	OPERAND cnt;

	/* Fixa till #1 operanden */
	cnt=cleanOperand();
	cnt.addrMode=AM_IMMEDIATE;
	cnt.size=SIZE_W;
	strcpy(cnt.constant.text,"1");

	newInstr(in,INSTR86_SUB,&cnt,&in->data.instr.op1,I_PREINSTR);
	newInstr(in,INSTR86_JNC,&in->data.instr.op2,NULL,I_SPECIAL);
	return 1;
}

/* dbcc
** ers„tts med
**	  Jcc tmplabel
**   SUB reg,1
**   JNC label
** tmplabel:
*/
static int dbcc(INPUT *in,TABLE *info)
{
	OPERAND cnt,tmpLabel;
	char label[256];

	sprintf(label,"%s_%d",objectName,in->lineNumber);

	/* Fixa till #1 operanden */
	cnt=cleanOperand();
	cnt.addrMode=AM_IMMEDIATE;
	cnt.size=SIZE_W;
	cnt.mode=OP_READ;
	strcpy(cnt.constant.text,"1");

	/* Fixa till tmpLabel operanden */
	tmpLabel=cleanOperand();
	tmpLabel.addrMode=AM_ABS_MEM;
	tmpLabel.mode=OP_FLOW;
	strcpy(tmpLabel.constant.text,label);

	newInstr(in,info->instr86,&tmpLabel,NULL,I_FLAGS);
	newInstr(in,INSTR86_SUB,&cnt,&in->data.instr.op1,I_PREINSTR);
	newInstr(in,INSTR86_JNC,&in->data.instr.op2,NULL,I_SPECIAL);
	newLabel(in,label);
	return 1;
}

/* dc
** db,dw,dd
*/
static int dc(INPUT *in,TABLE *info)
{
	switch(in->data.instr.size) {
	case SIZE_B:
		newInstr(in,INSTR86_DB,&in->data.instr.op1,&in->data.instr.op2,I_NORMAL);
		break;
	case SIZE_W:
		newInstr(in,INSTR86_DW,&in->data.instr.op1,&in->data.instr.op2,I_NORMAL);
		break;
	case SIZE_L:
		newInstr(in,INSTR86_DD,&in->data.instr.op1,&in->data.instr.op2,I_NORMAL);
		break;
	default:
		errorSize(in,in->data.instr.size);
		return 0;
	}
	return 1;
}

/* dcb, ds
** db dw dd ... dup (..)
*/
static int dcbDs(INPUT *in,TABLE *info)
{
	CONSTANT temp;
	CONSTANT old;
	char *p;
	old=in->data.instr.op1.constant;

	/* Fixa till operanden */
	strcpy(temp.text,strtok(old.text,","));
	strcat(temp.text," dup (");
	p=strtok(NULL,",");
	if(!p)
		p="?";
	strcat(temp.text,p);
	strcat(temp.text,")");
	in->data.instr.op1.constant=temp;

	return dc(in,info);
}

/* setZtoNC
** s„tt Z till !C mja sbb reg,reg, inneh†ll i register oves„ntligt
*/
static int setZtoNC(INPUT *in,TABLE *info)
{
	OPERAND tmp;
	if(in->flagsSet) {
		newInstr(in,info->instr86,&in->data.instr.op1,&in->data.instr.op2,I_PREINSTR);
		tmp=tmpReg();
		tmp.mode=OP_WRITE; /* Se till att regAss inte klagar... */
		tmp.size=SIZE_L;
		newInstr(in,INSTR86_SBB,&tmp,&tmp,I_FLAGS);
		return 1;
	} else return 0;
}

/* swap
** rotera register 16 steg (byt plats p† 16 ”vre och nedre bitarna)
*/
static int swap(INPUT *in,TABLE *info)
{
	OPERAND rot;
	rot=cleanOperand();
	rot.addrMode=AM_IMMEDIATE;
	rot.mode=OP_READ;
	strcpy(rot.constant.text,"16");
	newInstr(in,info->instr86,&rot,&in->data.instr.op1,I_FLAGS);
	return 1;
}

/* ext
** G”r en sign extend av ett register
*/
static int ext(INPUT *in,TABLE *info)
{
	OPERAND src,dst;
	src=in->data.instr.op1;
	dst=src;
	src.mode=OP_READ;
	dst.mode=OP_MODIFY;
	switch(in->data.instr.size) {
	case SIZE_W:
		src.size=SIZE_B;
		dst.size=SIZE_W;
		break;
	default:
		src.size=SIZE_W;
		dst.size=SIZE_L;
		break;
	}
	/* specialfall.. om op2 finns, anv„nd operanderna som de „r */
	if(in->data.instr.op2.addrMode!=AM_NO_OPERAND) {
		src=in->data.instr.op1;
		src.mode=OP_READ;
		dst=in->data.instr.op2;
		dst.mode=OP_WRITE;
	}
	newInstr(in,info->instr86,&src,&dst,I_FLAGS);
	return 1;
}

/* extb
** G”r en sign extend av ett register byte->long
*/
static int extb(INPUT *in,TABLE *info)
{
	OPERAND src,dst;
	src=in->data.instr.op1;
	dst=src;
	src.mode=OP_READ;
	dst.mode=OP_MODIFY;
	src.size=SIZE_B;
	dst.size=SIZE_L;
	newInstr(in,info->instr86,&src,&dst,I_FLAGS);
	return 1;
}

/* shift
** fixa till shift instruktioner med en operand
** dessa kan ha 1/2 operander, vid en operand
** „r den f”rsta operanden 1 och den andra den f”rsta...
*/
static int shift(INPUT *in,TABLE *info)
{
	OPERAND one;

	if(in->data.instr.op2.addrMode!=AM_NO_OPERAND)
		return 0;

	one=cleanOperand();
	one.addrMode=AM_IMMEDIATE;
	one.mode=OP_READ;
	strcpy(one.constant.text,"1");
	newInstr(in,info->instr86,&one,&in->data.instr.op1,I_FLAGS);
	return 1;
}

static int tst(INPUT *in,TABLE *info)
{
	OPERAND noll;

	noll=cleanOperand();
	noll.addrMode=AM_IMMEDIATE;
	strcpy(noll.constant.text,"0");
	newInstr(in,info->instr86,&noll,&in->data.instr.op1,I_FLAGS);
	return 1;
}

static int jump(INPUT *in,TABLE *info)
{
	OPERAND temp,addr;

	if(in->data.instr.op1.addrMode!=AM_INDIRECT)
		return 0;

	temp=tmpReg();
	addr=in->data.instr.op1;

	temp.mode=OP_WRITE;
	addr.mode=OP_READ;
	addr.size=SIZE_L;
	temp.size=SIZE_L;
	newInstr(in,INSTR86_LEA,&addr,&temp,I_PREINSTR);
	temp.mode=OP_FLOW;
	newInstr(in,info->instr86,&temp,NULL,I_NORMAL);
	return 1;
}

/* div beh”ver en or dst.w,dst.w om flaggor beh”vs */
static int divFlags(INPUT *in,TABLE *info)
{
	OPERAND op;

	if(!in->flagsSet) return 0;

	newInstr(in,info->instr86,&in->data.instr.op1,&in->data.instr.op2,I_PREINSTR);
	op=in->data.instr.op2;
	op.size=SIZE_W;
	newInstr(in,INSTR86_OR,&op,&op,I_SPECIALFLAGS);
	return 1;
}

/* mul beh”ver en or dst.l,dst.l om flaggor beh”vs */
static int mulFlags(INPUT *in,TABLE *info)
{
	OPERAND op;

	if(!in->flagsSet) return 0;

	newInstr(in,info->instr86,&in->data.instr.op1,&in->data.instr.op2,I_PREINSTR);
	op=in->data.instr.op2;
	op.size=SIZE_L;
	newInstr(in,INSTR86_OR,&op,&op,I_SPECIALFLAGS);
	return 1;
}


/* include
** prj: -> \
** / -> \
** .? -> .ipc
** .bin -> .bpc
** ingen s”kv„g -> addera s”kv„g
*/
static int include(INPUT *in,TABLE *info)
{
	CONSTANT name;
	char *ptr;

	memset(&name,0,sizeof(name));

	ptr=in->data.instr.op1.constant.text;

	/* Fixa "PRJ:" */
	if(strncasecmp(ptr,"PRJ:",4)==0) {
		strcat(name.text,"\\");
		ptr+=4;
	}
	strcat(name.text,ptr);

	/* .? -> .?pc */
	if(strlen(name.text)>=2 && name.text[strlen(name.text)-2]=='.')
		strcat(name.text,"pc");

	/* .bin -> .bpc */
	if(strlen(name.text)>=4)
		ptr=&name.text[strlen(name.text)-4];
		if(strcasecmp(ptr,".bin")==0)
			strcpy(ptr,".bpc");

	/* / -> \ */
	for(ptr=name.text+strcspn(name.text,"/");*ptr;ptr+=strcspn(ptr,"/"))
		*ptr='\\';

	if(strchr(name.text,'\\'))
		in->data.instr.op1.constant=name;
	else
		sprintf(in->data.instr.op1.constant.text,"%s%s",objectPath,name.text);

	return 0;
}
/* rs.b -> RSTRUCB
** rs.w -> RSTRUCW
** rs.l -> RSTRUCD
*/
static int rs(INPUT *in,TABLE *info)
{
	switch(in->data.instr.size) {
	case SIZE_B:
		newStdInstr(in,INSTR86_RSTRUCB);
		break;
	case SIZE_W:
		newStdInstr(in,INSTR86_RSTRUCW);
		break;
	case SIZE_L:
		newStdInstr(in,INSTR86_RSTRUCD);
		break;
	default:
		errorSize(in,in->data.instr.size);
		return 0;
	}
	return 1;
}

/* movem
** movem till register skall g”ra sign extend vid behov
*/
static int movem(INPUT *in,TABLE *info)
{
	OPERAND opl;

	/* Anger storleken att sign extend kan beh”vas? */
	switch(in->data.instr.size) {
	case SIZE_L:
		/* F”r long beh”ver vi aldrig g”ra n†got */
		return 0;
	default:
		/* Vi kan beh”va g”ra sign extend om det „r till register */
		switch(in->data.instr.op2.addrMode) {
		case AM_REG:
			/* Det „r endast om dst „r register som vi beh”ver bry oss */
			opl=in->data.instr.op2;
			opl.size=SIZE_L;
			newInstr(in,INSTR86_MOVSX,&in->data.instr.op1,&opl,I_NORMAL);
			return 1; /* Allt „r klart. */
		default:
			/* Nehepp.. det „r inte n†got special fall */
			return 0;
		}
	}
}

/* Push m†ste justera stacken j„mt 2 */
static int push(INPUT *in,TABLE *info)
{
	OPERAND sp;
	OPERAND spadr;


	/* Det „r endast SIZE_B som „r speciell */
	if(in->data.instr.size!=SIZE_B)
		return 0;

	/* Jaha.. d† var det en byte push, d† m†ste f”rst stackpekaren
	** r„knas ned med ett..
	*/

	/* Stackpekaren var det ja... */
	sp=cleanOperand();
	sp.addrMode=AM_REG;
	sp.reg=REG_SP;

	/* Och ned med ett var det ocks†... */
	spadr=sp;
	spadr.addrMode=AM_INDIRECT;
	strcpy(spadr.constant.text,"-1");

	/* D† g”r vi en lea f”r att f† till sp... */
	newInstr(in,INSTR86_LEA,&spadr,&sp,I_PREINSTR);

	/* Slut, †terg†ng till det normala... */
	return 0;
}

/* Pull m†ste justera stacken j„mt 2, och g”ra en sign extend om size<L */
static int pull(INPUT *in,TABLE *info)
{
	OPERAND data,noop,dst;

	/* Det „r endast SIZE_L som inte „r speciell */
	if(in->data.instr.size==SIZE_L)
		return 0;

	/* pull */
	data=tmpReg();
	data.size=in->data.instr.size;
	if(data.size==SIZE_B)
		data.size=SIZE_W;	/* Justering av stack pekaren... */
	data.mode=OP_WRITE;
	noop=cleanOperand();
	newInstr(in,info->instr86,&data,&noop,I_NORMAL);
	data.size=in->data.instr.size;

	/* Sign extend */
	dst=in->data.instr.op1;
	dst.size=SIZE_L;
	newInstr(in,INSTR86_MOVSX,&data,&dst,I_POSTINSTR);

	/* Thats all folks! */
	return  1;
}

/********************************************/
/* Matcha 68000 mot 80386 */
static void doMatch86(void)
{
	INPUT *in;
	TABLE *info=NULL;
	in=READER();
	if(!in)
		return;

	switch(in->type) {
	case IS_INSTR:
		info=lookup(in->data.instr.instr);
		if(!info) {
			errorUnimpl(in);
		} else {
			/* „r det ett specialfall? */
			if(info->special && info->special(in,info))
				return; /* specialfalls rutinen fixa det */
			/* om inte specialfall */
			newStdInstr(in,info->instr86);
		}
		break;
	default:
		output((OUTPUT *)in);
	}
}


/* Returnera n„sta element i k”n, generera mera om det beh”vs */
OUTPUT *match86(void)
{
	if(curritem>=nitems) {
		resetOutput();
		doMatch86();
	}
	if(curritem<nitems)
		return &out[curritem++];
	else
		return NULL;
}
