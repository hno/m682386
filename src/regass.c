/* Register tilldelning/optimering
**
** Detta „r inte n†got l„tt kapitel...
**
** Finns inte det beg„rda 68000 registret m†ste det tilldelas
** ett 386 register, och skall registret anv„ndas f”r l„sning
** m†ste 386 registret laddas.
**
** Ett 386 register beh†lls s† l„nge det aktuella 68000 registret
** anv„nds inom blocket
**
** Ett block „r ett omr†de mellan tv† fl”deskontroll punkter.
**
** Fl”deskontroll punkter „r:
** o Labels
** o Instruktioner som f”r„ndrar fl”det
** o Data direktiv (Detta f”r att programmet inte skall bli
**   ”verbelastat...)
** o ™vriga instruktioner som skall hanteras speciellt
**
**
** Vad som skall g”ras om 386 registren tar slut f†r kluras
** ut vid ett senare tillf„lle....
**
*/

/* F”rsta versionen:
**   Ingen vidare optimering.
**   1. Tilldela de register som skall l„sas/modifieras
**   2. Instruktion
**   3. Skriv ut alla modifieringar
** Registertilldelningen cacheas, och t”ms vid fl”despunkter
** dvs s†dana punkter som man kan hoppa till (labels)
*/

/* Ev framtida optimerande variant:
** Lite snabbt om hur det skall fungera
** 1. L„s in ett block, till n„sta kontrollpunkt
** 2. Expandera s† att 68000 register anv„ndningen „r l„tttolkad
**    o Vilka 68000 register som avl„ses
**    o Vilka 68000 register som s„tts
** 3. Klura ut hur 68000 registren anv„nds
**    1. Jobba nedifr†n och fyll i att de 68000 register som anv„nds
**       „r aktuella tills en instruktion som p†verkar dessa p†tr„ffas
**    2. Jobba uppifr†n och ta bort anv„ndningsinfo som ifyllts av
** 		steg 1, f”r de register som inte s„tts innan de anv„nds
** 4. Jobba instruktion f”r instruktion
**    1. Vad beh”vs det f”r k„ll register?
**		3. Vad beh”vs det f”r destinations register?
**		4. Generera instruktion
**		5. Frig”r register som inte beh”ver beh†llas
**
**		H„r kan det ev optimeras en del, s† att om destinationen
**		„r ett register, s† kan k„llregister frig”ras s† att de
**		kan anv„ndas som destinations register, men detta „r inte
**		helt trivialt, s† det f†r vara s† l„nge
**
**		Vidare s† kr„ver mul och div specialbehandling
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "regass.h"

/* Hur m†nga nya instruktioner som kan genereras av en input... */
#define MAX_OUTPUT 20

/* Varifr†n f†r vi v†r input? */
#include "match86.h"
#define READER match86
typedef MATCH86 INPUT;
typedef REGASS OUTPUT;

/* Output k” */
static OUTPUT out[MAX_OUTPUT];
static int nitems=0;
static int curritem=0;

static void output(OUTPUT *);

/* L„gg till ett element i ouput k”n */
static void output(OUTPUT *elem)
{
	if(nitems>=MAX_OUTPUT) {
		fprintf(stderr,"REGASS: Output que full in line %d\n",elem->lineNumber);
		exit(1);
	}
	out[nitems++]=*elem;
}

/* terst„ll output k”n */
static void resetOutput(void)
{
	/* lite felkoll skadar aldrig... */
	if(curritem<nitems) {
		fprintf(stderr,"REGASS: resetOutput on nonempty output queue!!! lastline:%d\n"
			,out[nitems-1].lineNumber);
		exit(1);
	}
	/* Ok.. t”m output k”n */
	curritem=0;
	nitems=0;
}

static void errorTmpUse(void)
{
	OUTPUT out;
	memset(&out,0,sizeof(out));
	out.type=IS_REGASS_ERROR;
	out.data.error.error=R_E_TMPUSE;
	output(&out);
}

static void errorOpMode(OPMODE mode)
{
	OUTPUT out;
	memset(&out,0,sizeof(out));
	out.type=IS_REGASS_ERROR;
	out.data.error.error=R_E_OPMODE;
	out.data.error.opMode=mode;
	output(&out);
}

static void errorAddrMode(ADDRMODE mode)
{
	OUTPUT out;
	memset(&out,0,sizeof(out));
	out.type=IS_REGASS_ERROR;
	out.data.error.error=R_E_ADDRMODE;
	out.data.error.addrMode=mode;
	output(&out);
}

static void errorRegUse(REGISTER reg)
{
	OUTPUT out;
	memset(&out,0,sizeof(out));
	out.type=IS_REGASS_ERROR;
	out.data.error.error=R_E_REGUSE;
	out.data.error.reg=reg;
	output(&out);
}

static void errorSize(SIZE size)
{
	OUTPUT out;
	memset(&out,0,sizeof(out));
	out.type=IS_REGASS_ERROR;
	out.data.error.error=R_E_SIZE;
	out.data.error.size=size;
	output(&out);
}


/* Generera en tom operand */
OPERAND cleanOperand(void)
{
	OPERAND op;
	memset(&op,0,sizeof(op));
	op.addrMode=AM_UNKNOWN;
	return op;
}

/* Generera en tom operand */
OPERAND noOperand(void)
{
	OPERAND op=cleanOperand();
	op.addrMode=AM_NO_OPERAND;
	return op;
}

/* flytta in 68000 register i 386 register, eller vise versa */
static void setReg(REGISTER dst,REGISTER src)
{
	OPERAND op1,op2;
	OUTPUT out;

	if(ISTEMPREG(src)) {
		/* Ingen ide att s„tta TEMP, men generera en varning utifall att */
		errorTmpUse();
		return;
	}

	if(ISTEMPREG(dst))
		return;	 /* Ingen ide att spara TEMP */

	if(dst==REG_SP)
		return;	/* Ingen ide att spara SP, eftersom den alltid finnes... */

	if(src==REG_SP)
		return;	/* Ingen ide att s„tta SP, eftersom den alltid finnes... */

	op1=cleanOperand();
	op1.addrMode=AM_REG;
	op1.reg=dst;
	op1.size=SIZE_L;
	op1.mode=OP_WRITE;
	op2=op1;
	op2.reg=src;
	op2.mode=OP_READ;
	memset(&out,0,sizeof(out));
	out.type=IS_INSTR;
	out.data.instr.instr=INSTR86_MOV;
	out.data.instr.opsMode=O_SPECIAL;
	out.data.instr.op1=op1;
	out.data.instr.op2=op2;
	output(&out);
}

/* Kontroll om det „r en fl”deskontrollerande punkt */
static int isFlow(INPUT *in)
{
	switch(in->type) {
	case IS_LABEL:
		return 1;
	case IS_INSTR:
		switch(in->data.instr.instr) {
		case INSTR86_CALL:
		case INSTR86_JMP:
		case INSTR86_RET:
			return 1;
		default:
			switch(in->data.instr.opsMode) {
			case O_SPECIAL:
				return 1;
			default:
				return 0;
			}
		}
	default:
		return 0;
	}
}

static struct {
	REGISTER reg86;
	REGISTER reg68;
	long age;
	SIZE minSize;
} regUse[]={
	{REG_ESI,REG_NONE,0,SIZE_W},
	{REG_EDI,REG_NONE,0,SIZE_W},
	{REG_EBP,REG_NONE,0,SIZE_W},
	{REG_EBX,REG_NONE,0,SIZE_B},
	{REG_ECX,REG_NONE,0,SIZE_B},
	{REG_EDX,REG_NONE,0,SIZE_B},
	{REG_EAX,REG_NONE,0,SIZE_B},
	{REG_NONE}
};

/* T”m cachen */
static void flushCache(void)
{
	int i;
	for(i=0;regUse[i].reg86!=REG_NONE;i++) {
		regUse[i].reg68=REG_NONE;
		regUse[i].age=0;
	}
}

/* ldra cachen */
static void ageCache(void)
{
	int i;
	for(i=0;regUse[i].reg86!=REG_NONE;i++)
		regUse[i].age+=1;
}

static void clearCachePos(int i)
{
	long maxAge;
	int j;
	for(maxAge=0,j=0;regUse[j].reg86!=REG_NONE;j++)
		maxAge=max(maxAge,regUse[j].age);

	regUse[i].age=maxAge+1;
	regUse[i].reg68=REG_NONE;
}

static void lockReg86(REGISTER reg)
{
	int i;
	for(i=0;regUse[i].reg86!=REG_NONE;i++) {
		if(regUse[i].reg86==reg) {
			regUse[i].reg68=REG_NONE;
			if(regUse[i].age<0) {
				fprintf(stderr,"REGASS: Locking a locked register %d\n",reg);
				exit(1);
			}
			regUse[i].age=-99999999L;	 /* Gigantiskt l†ngt fram i tiden.. s† att den inte kommer att anv„ndas innan den „r frigjord */
		}
	}
}

static void freeReg86(REGISTER reg)
{
	int i;
	for(i=0;regUse[i].reg86!=REG_NONE;i++) {
		if(regUse[i].reg86==reg)
			clearCachePos(i);
	}
}

static REGISTER getReg(REGISTER reg,SIZE size)
{
	int i;
	long maxAge;

	/* L†t cachen †ldras lite... */
	ageCache();

	/* Klura ut hur gammal den „ldsta „r */
	for(i=0,maxAge=0;regUse[i].reg86!=REG_NONE;i++) {
		if(size>=regUse[i].minSize)
			maxAge=max(maxAge,regUse[i].age);
	}

	/* Avtrubbning av †lder till 15 */
	maxAge=min(maxAge,15);

	/* S†, nu tar vi den f”rste som „r tillr„ckligt gammal */
	for(i=0;regUse[i].age<maxAge || size<regUse[i].minSize;i++);
	regUse[i].reg68=reg;
	regUse[i].age=0;

	return regUse[i].reg86;
}

/* Kontrollerar om ett register redan finns i cachen.
** om registret finns, men inte g†r att anv„nda p† grund
** av att 386 registret inte kan adresseras i r„tt storlek
** flyttas det till ett annat register
*/
static REGISTER inCache(REGISTER reg,SIZE size)
{
	int i;
	for(i=0;regUse[i].reg86!=REG_NONE;i++) {
		if(regUse[i].reg68==reg) {
			if(size<regUse[i].minSize) {
				reg=getReg(reg,size);
				setReg(reg,regUse[i].reg86);
				clearCachePos(i);
				return reg;
			} else {
				regUse[i].age=0;
				return regUse[i].reg86;
			}
		}
	}
	return REG_NONE;
}

/* Spara ett 386 register i ett annat 386 register */
static void saveReg86(REGISTER reg86,REGISTER reg68,SIZE size)
{
	REGISTER nreg86;
	lockReg86(reg86);
	nreg86=getReg(reg68,size);
	freeReg86(reg86);
	setReg(nreg86,reg86);
}

static void lockNSaveReg86Ops(REGISTER reg,OPERAND op1,OPERAND op2)
{
	REGISTER regs[10];
	SIZE sizes[10];
	int nregs;
	int i;

	nregs=0;

#define ADDREG(reg,size)	(regs[nregs]=reg,sizes[nregs]=size,nregs++)

	/* Operand 1 */
	switch(op1.addrMode) {
	case AM_REG:
		ADDREG(op1.reg,op1.size);
		break;
	case AM_INDIRECT:
		ADDREG(op1.reg,SIZE_L);
		ADDREG(op1.index,op1.indexSize);
	default:
		break;
	}

	/* Operand 2 */
	switch(op2.addrMode) {
	case AM_REG:
		ADDREG(op2.reg,op2.size);
		break;
	case AM_INDIRECT:
		ADDREG(op2.reg,SIZE_L);
		ADDREG(op2.index,op2.indexSize);
	default:
		break;
	}

	for(i=0;i<nregs;i++) {
		if(regs[i] && inCache(regs[i],SIZE_L)==reg)
			saveReg86(reg,regs[i],sizes[i]);
	}
	lockReg86(reg);
}

static REGISTER assignReg(REGISTER reg,OPMODE mode,SIZE size)
{
	REGISTER new;
	if(reg==REG_NONE)
		return REG_NONE;

	if(reg==REG_PC)	/* PC beh”vs aldrig */
		return REG_NONE;

	/* SP „r ALLTID ESP */
	if(reg==REG_SP)
		return REG_ESP;

	if(((reg<REG_D0) || (reg>REG_A6)) && !ISTEMPREG(reg)) {
		errorRegUse(reg);
		return reg;
	}

	/* Fixa default storlek */
	if(size<SIZE_B) {
		if(reg>=REG_A0 && reg<=REG_A7)
			size=SIZE_L;
		else
			size=SIZE_B;
	}

	/* Finns den redan i cachen */
	new=inCache(reg,size);

	/* om den inte fanns i cachen m†ste vi fixa den.. */
	if(new==REG_NONE) {
		/* den fanns inte i cachen */
		new=getReg(reg,size);
		switch(mode) {
		case OP_MODIFY:
		case OP_READ:
			setReg(new,reg);
			break;
		case OP_WRITE:
			if(size!=SIZE_L && reg!=REG_TEMP && reg!=REG_TEMP_INDEX)
				setReg(new,reg);
			break;
		default:
			errorOpMode(mode);
			setReg(new,reg);
		}
	}
	return new;
}

static OPERAND regAssOperand(OPERAND op)
{
	if(!op.indexSize)
		op.indexSize=SIZE_W;
	switch(op.addrMode) {
	case AM_NO_OPERAND:
		break;
	case AM_REG:
		op.reg=assignReg(op.reg,op.mode,op.size);
		break;
	case AM_INDIRECT:
		op.reg=assignReg(op.reg,OP_READ,SIZE_L);
		op.index=assignReg(op.index,OP_READ,op.indexSize);
		break;
	case AM_ABS_MEM:
		break;
	case AM_IMMEDIATE:
		break;
	case AM_SPECIAL:
		break;
	default:
		errorAddrMode(op.addrMode);
		op.reg=assignReg(op.reg,OP_MODIFY,SIZE_B);
		op.index=assignReg(op.index,OP_MODIFY,op.indexSize);
		break;
	}
	return op;
}

/* Tilldela register f”r en instruktion */
static INSTR86 regAssInstr(INSTR86 instr)
{
	ageCache();
	instr.op2=regAssOperand(instr.op2);
	instr.op1=regAssOperand(instr.op1);
	return instr;
}

static void storeChanges(INSTR86 instr)
{
	/* Operand 1 */
	switch(instr.op1.mode) {
	case OP_MODIFY:
	case OP_WRITE:
		if(instr.op1.addrMode==AM_REG)
			setReg(instr.op1.reg,inCache(instr.op1.reg,SIZE_L));
		break;
	default:
		break;
	}
	/* Operand 2 */
	switch(instr.op2.mode) {
	case OP_MODIFY:
	case OP_WRITE:
		if(instr.op1.addrMode==AM_REG)
			setReg(instr.op2.reg,inCache(instr.op2.reg,SIZE_L));
		break;
	default:
		break;
	}
}

static void doInstruction(INPUT *in)
{
	OUTPUT out;
	*(INPUT *)&out=*in;
	out.data.instr=regAssInstr(in->data.instr);
	output(&out);
}

/* genMove2Reg
** generara en mov reg,<ea>
*/
static OPERAND genMove2Reg(INPUT *in,REGISTER reg,OPERAND op)
{
	OUTPUT out;

	memset(&out,0,sizeof(out));
	out.type=IS_INSTR;
	out.lineNumber=in->lineNumber;
	out.flagsPreserved=F_ALL;
	out.data.instr.instr=INSTR86_MOV;
	out.data.instr.op2=op;
	out.data.instr.op1.addrMode=AM_REG;
	out.data.instr.op1.reg=reg;
	out.data.instr.op1.size=out.data.instr.op2.size;
	output(&out);

	/* returnera en operand som „r registret */
	return out.data.instr.op1;

}

static void doSpecialDiv(INPUT *in)
{
	/*          		   op2  op1
	** 			divs.l	<ea>,Dn
	** lockReg(eax)
	** lockReg(edx)
	** mov eax,Dn
	** mov edx,Dn
	** shr edx,16	; dx=h”ga bitarna
	** instr <ea>.w
	** shl edx,16
	** mov dx,ax
	** mov Dn,edx
	** freeReg(eax)
	** freeReg(eax)
	*/

	OUTPUT out;
	OPERAND op1,op2,regDX,regAX,regEDX,regEAX,sexton;

	/* F”rst lite grundl„ggande kontroll.... */
	switch(in->data.instr.size) {
	case SIZE_W:
		break;
	default:
		errorSize(in->data.instr.size);
		break;
	}
	op1=in->data.instr.op1;
	op2=in->data.instr.op2;
	op1.size=SIZE_L;
	op2.size=SIZE_W;

	sexton=cleanOperand();
	sexton.addrMode=AM_IMMEDIATE;
	strcpy(sexton.constant.text,"16");

	regDX=cleanOperand();
	regDX.addrMode=AM_REG;
	regDX.reg=REG_EDX;
	regDX.size=SIZE_W;
	regAX=regDX;
	regAX.reg=REG_EAX;
	regAX.size=SIZE_W;
	regEDX=regDX;
	regEDX.size=SIZE_L;
	regEAX=regAX;
	regEAX.size=SIZE_L;

	lockNSaveReg86Ops(REG_EAX,op1,op2);
	lockNSaveReg86Ops(REG_EDX,op1,op2);

	/* Fixa till en instruktion */
	memset(&out,0,sizeof(out));
	out.type=IS_INSTR;
	out.data.instr.opsMode=O_SPECIAL;

	/* Flytta in Dn i EAX */
	op1.size=SIZE_L;
	op1.mode=OP_READ;
	regEAX.mode=OP_WRITE;
	out.data.instr.instr=INSTR86_MOV;
	out.data.instr.op1=regEAX;
	out.data.instr.op2=regAssOperand(op1);
	output(&out);

	/* Flytta in Dn i EDX */
	op1.size=SIZE_L;
	op1.mode=OP_READ;
	regEDX.mode=OP_WRITE;
	out.data.instr.instr=INSTR86_MOV;
	out.data.instr.op1=regEDX;
	out.data.instr.op2=regAssOperand(op1);
	output(&out);

	/* Shifta ned de h”ga bitarna i DX */
	regEDX.mode=OP_MODIFY;
	out.data.instr.instr=INSTR86_SHR;
	out.data.instr.op1=regEDX;
	out.data.instr.op2=sexton;
	output(&out);

	/* Dags f”r instruktionen */
	out.data.instr.op1=regAssOperand(op2);
	out.data.instr.op2=noOperand();
	out.data.instr.instr=in->data.instr.instr;
	output(&out);

	/* Shifta EDX 16 steg */
	out.data.instr.op1=regEDX;
	out.data.instr.op2=sexton;
	out.data.instr.instr=INSTR86_SHL;
	output(&out);

	/* Flytta in AX i DX */
	out.data.instr.op1=regDX;
	out.data.instr.op2=regAX;
	out.data.instr.instr=INSTR86_MOV;
	output(&out);

	/* EAX Beh”vs inte l„ngre, eftersom resultatet nu finns i EDX */
	freeReg86(REG_EAX);

	/* Flytta resultatet(EDX) i Dn */
	op1.mode=OP_WRITE;
	op1.size=SIZE_L;
	regEDX.mode=OP_READ;
	out.data.instr.instr=INSTR86_MOV;
	out.data.instr.op1=regAssOperand(op1);
	out.data.instr.op2=regEDX;
	output(&out);

	/* EDX beh”vs inte l„ngre, eftersom resultatet nu „r sparat */
	freeReg86(REG_EDX);
}


/* doSpecialMul
** fixa en mul instruktion
**					   op2  op1
** 			mul?	<ea>,Dn
**	mov?x	Dn,Dn
** if <ea>!=AM_IMMEDIATE
**		mov?x	TEMP,<ea>
**		<ea>=TEMP
** fi
** instr=IMUL;
** regAssInstr(instr)
*/
static void doSpecialMul(INPUT *in)
{
	OPERAND op1,op1l;
	OPERAND op2,op2l;
	OUTPUT out;

	op1=in->data.instr.op1;
	op2=in->data.instr.op2;

	/* Fixa till en instruktion */
	memset(&out,0,sizeof(out));
	out.type=IS_INSTR;
	out.data.instr.opsMode=O_SPECIAL;
	switch(in->data.instr.instr) {
	case INSTR86_MUL:	/* Unsigned */
		out.data.instr.instr=INSTR86_MOVZX;
		break;
	case INSTR86_IMUL:/* Signed */
		out.data.instr.instr=INSTR86_MOVSX;
		break;
	default:
		break;
	}

	/* MOV?X op1.L,op1.W ; G”r en extend av Dn(op1) */
	op1l=op1;
	op1l.size=SIZE_L;
	out.data.instr.op1=regAssOperand(op1l);
	op1.size=SIZE_W;
	out.data.instr.op2=regAssOperand(op1);
	output(&out);
	in->data.instr.op1.size=SIZE_L;

	/* MOV?X REG_TEMP,op2.W ; G”r en extend av op2 */
	if(op2.addrMode!=AM_IMMEDIATE) {
		op2l=cleanOperand();
		op2l.addrMode=AM_REG;
		op2l.size=SIZE_L;
		op2l.reg=REG_TEMP;
		op2l.mode=OP_WRITE;
		out.data.instr.op1=regAssOperand(op2l);
		op2.size=SIZE_W;
		op2.mode=OP_READ;
		out.data.instr.op2=regAssOperand(op2);
		output(&out);
		op2l.mode=OP_READ;
		in->data.instr.op2=op2l; /* OBS! op2l ej registertilldelad */
	}

	/* S†, d† skall det vara klart. */
	in->data.instr.instr=INSTR86_IMUL;
	doInstruction(in);
}

/* Add Packed Binary Coded Decimal
** Mov AL,op1
** ADC AL,op2
** DAA
** MOV op1,al
*/
static void doSpecialAbcd(INPUT *in)
{
	OUTPUT out;
	OPERAND regAL;

	out=*(OUTPUT *)in;

	lockNSaveReg86Ops(REG_EAX,in->data.instr.op1,in->data.instr.op2);
//	lockReg86(REG_EAX);

	regAL=cleanOperand();
	regAL.addrMode=AM_REG;
	regAL.reg=REG_EAX;
	regAL.size=SIZE_B;

	/* MOV AL,op1 */
	in->data.instr.op1.mode=OP_READ;
	out.data.instr.instr=INSTR86_MOV;
	out.data.instr.op2=regAssOperand(in->data.instr.op1);
	regAL.mode=OP_WRITE;
	out.data.instr.op1=regAL;
	output(&out);

	/* ADC AL,op2 */
	out.data.instr.instr=INSTR86_ADC;
	out.data.instr.op2=regAssOperand(in->data.instr.op2);
	output(&out);

	/* DAA */
	out.data.instr.instr=INSTR86_DAA;
	out.data.instr.op1=noOperand();
	out.data.instr.op2=noOperand();
	output(&out);

	/* MOV op1,AL */
	out.data.instr.instr=INSTR86_MOV;
	out.data.instr.op1=regAssOperand(in->data.instr.op1);
	out.data.instr.op2=regAL;
	output(&out);

	freeReg86(REG_EAX);
}

/* Sub Packed Binary Coded Decimal
** Mov AL,op1
** SBB AL,op2
** DAS
** MOV op1,al
*/
static void doSpecialSbcd(INPUT *in)
{
	OUTPUT out;
	OPERAND regAL;

	out=*(OUTPUT *)in;

	lockNSaveReg86Ops(REG_EAX,in->data.instr.op1,in->data.instr.op2);

	regAL=cleanOperand();
	regAL.addrMode=AM_REG;
	regAL.reg=REG_EAX;
	regAL.size=SIZE_B;

	/* MOV AL,op1 */
	in->data.instr.op1.mode=OP_READ;
	out.data.instr.instr=INSTR86_MOV;
	out.data.instr.op2=regAssOperand(in->data.instr.op1);
	regAL.mode=OP_WRITE;
	out.data.instr.op1=regAL;
	output(&out);

	/* SBB AL,op2 */
	out.data.instr.instr=INSTR86_SBB;
	out.data.instr.op2=regAssOperand(in->data.instr.op2);
	output(&out);

	/* DAS */
	out.data.instr.instr=INSTR86_DAS;
	out.data.instr.op1=noOperand();
	out.data.instr.op2=noOperand();
	output(&out);

	/* MOV op1,AL */
	out.data.instr.instr=INSTR86_MOV;
	in->data.instr.op1.mode=OP_WRITE;
	out.data.instr.op1=regAssOperand(in->data.instr.op1);
	out.data.instr.op2=regAL;
	output(&out);

	freeReg86(REG_EAX);
}

/* roterings och shift instruktioner */
static int doSpecialShift(INPUT *in)
{
	OUTPUT out;

	/* r det en immediate s† beh”ver vi inte g”ra n†got */
	if(in->data.instr.op2.addrMode==AM_IMMEDIATE)
		return 0;

	/* Kopiera instruktionen med allt viktigt... */
	*(INPUT *)&out=*in;


	/* vi beh”ver ECX f”r eget bruk... */
	lockNSaveReg86Ops(REG_ECX,in->data.instr.op1,in->data.instr.op2);

	out.data.instr.op1=regAssOperand(out.data.instr.op1);
	out.data.instr.op2.size=SIZE_B;
	out.data.instr.op2=genMove2Reg(in,REG_ECX,regAssOperand(out.data.instr.op2));
	output(&out);

	freeReg86(REG_ECX);

	return 1;
}


/* Special instruktioner */
static int doSpecial(INPUT *in)
{
	switch(in->data.instr.instr) {
	case INSTR86_IDIV:	/*	Signed divide */
	case INSTR86_DIV:		/* Unsigned divide */
		doSpecialDiv(in);
		return 1;
	case INSTR86_IMUL:	/* Signed multiply */
	case INSTR86_MUL:		/* Unsigned multiply */
		doSpecialMul(in);
		return 1;
	case INSTR86_INT_ABCD:	/* Add Packed Binary Coded Decimal */
		doSpecialAbcd(in);
		return 1;
	case INSTR86_INT_SBCD:	/* Sub Packed Binary Coded Decimal */
		doSpecialSbcd(in);
		return 1;
	case INSTR86_SAL:
	case INSTR86_SAR:
	case INSTR86_SHL:
	case INSTR86_SHR:
	case INSTR86_SHLD:
	case INSTR86_SHRD:
	case INSTR86_ROL:
	case INSTR86_ROR:
	case INSTR86_RCL:
	case INSTR86_RCR:
		return doSpecialShift(in);
	default:
		return 0;
	}
}

/* Jobba lite */
static void doIt(void)
{
	INPUT *in;
	in=READER();
	if(!in) return;

	switch(in->type) {
	case IS_INSTR:
		if(!doSpecial(in)) {
			doInstruction(in);
		}
		storeChanges(in->data.instr);
		break;
	default:
		output((OUTPUT *)in);
	}

	if(isFlow(in))
		flushCache();
}


/* Returnera n„sta element i k”n, generera mera om det beh”vs */
OUTPUT *regAss(void)
{
	if(curritem>=nitems) {
		resetOutput();
		doIt();
	}
	if(curritem<nitems)
		return &out[curritem++];
	else
		return NULL;
}
