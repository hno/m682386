#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"

/********************* data ************************************/

/* parserns interna dynamiska information */
static PARSER_STATE state;

/* instruktionstabell f”r ”vers„ttning av instruktionsnamn till koder */
static struct instrName {
	char name[10];
	INSTRCODE68 code;
} instrtab[]= {
	/* 68020 instruktioner */
	{"ABCD",INSTR68_ABCD},
	{"ADD",INSTR68_ADD},
	{"ADDA",INSTR68_ADD},
	{"ADDI",INSTR68_ADD},
	{"ADDQ",INSTR68_ADDQ},
	{"ADDX",INSTR68_ADDX},
	{"AND",INSTR68_AND},
	{"ANDI",INSTR68_AND},
	{"ASL",INSTR68_ASL},
	{"ASR",INSTR68_ASR},
	{"BRA",INSTR68_BRA},
	{"BHI",INSTR68_BHI},
	{"BLS",INSTR68_BLS},
	{"BCC",INSTR68_BCC},
	{"BCS",INSTR68_BCS},
	{"BNE",INSTR68_BNE},
	{"BEQ",INSTR68_BEQ},
	{"BVC",INSTR68_BVC},
	{"BVS",INSTR68_BVS},
	{"BPL",INSTR68_BPL},
	{"BMI",INSTR68_BMI},
	{"BGE",INSTR68_BGE},
	{"BLT",INSTR68_BLT},
	{"BGT",INSTR68_BGT},
	{"BLE",INSTR68_BLE},
	{"BCHG",INSTR68_BCHG},
	{"BCLR",INSTR68_BCLR},
	{"BFCHG",INSTR68_BFCHG},
	{"BFCLR",INSTR68_BFCLR},
	{"BFEXTS",INSTR68_BFEXTS},
	{"BFEXTU",INSTR68_BFEXTU},
	{"BFFFO",INSTR68_BFFFO},
	{"BFINS",INSTR68_BFINS},
	{"BFSET",INSTR68_BFSET},
	{"BFTST",INSTR68_BFTST},
	{"BKPT",INSTR68_BKPT},
	{"BSET",INSTR68_BSET},
	{"BSR",INSTR68_BSR},
	{"BTST",INSTR68_BTST},
	{"CALL",INSTR68_CALL},
	{"CALLM",INSTR68_CALLM},
	{"CAS",INSTR68_CAS},
	{"CAS2",INSTR68_CAS2},
	{"CHK",INSTR68_CHK},
	{"CHK2",INSTR68_CHK2},
	{"CLR",INSTR68_CLR},
	{"CMP",INSTR68_CMP},
	{"CMP2",INSTR68_CMP2},
	{"CMPA",INSTR68_CMP},
	{"CMPI",INSTR68_CMP},
	{"CMPM",INSTR68_CMP},
	{"DBT",INSTR68_DBT},
	{"DBF",INSTR68_DBF},
	{"DBRA",INSTR68_DBF},
	{"DBHI",INSTR68_DBHI},
	{"DBLS",INSTR68_DBLS},
	{"DBCC",INSTR68_DBCC},
	{"DBCS",INSTR68_DBCS},
	{"DBHS",INSTR68_DBCC},
	{"DBLO",INSTR68_DBCS},
	{"DBNE",INSTR68_DBNE},
	{"DBEQ",INSTR68_DBEQ},
	{"DBVC",INSTR68_DBVC},
	{"DBVS",INSTR68_DBVS},
	{"DBPL",INSTR68_DBPL},
	{"DBMI",INSTR68_DBMI},
	{"DBGE",INSTR68_DBGE},
	{"DBLT",INSTR68_DBLT},
	{"DBGT",INSTR68_DBGT},
	{"DBLE",INSTR68_DBLE},
	{"DIVS",INSTR68_DIVS},
	{"DIVSL",INSTR68_DIVSL},
	{"DIVU",INSTR68_DIVU},
	{"DIVUL",INSTR68_DIVUL},
	{"EOR",INSTR68_EOR},
	{"EORI",INSTR68_EOR},
	{"EXG",INSTR68_EXG},
	{"EXT",INSTR68_EXT},
	{"EXTB",INSTR68_EXTB},
	{"ILLEGAL",INSTR68_ILLEGAL},
	{"JMP",INSTR68_JMP},
	{"JSR",INSTR68_JSR},
	{"LEA",INSTR68_LEA},
	{"LINK",INSTR68_LINK},
	{"LSL",INSTR68_LSL},
	{"LSR",INSTR68_LSR},
	{"MOVE",INSTR68_MOVE},
	{"MOVEA",INSTR68_MOVE},
	{"MOVEC",INSTR68_MOVEC},
	{"MOVEM",INSTR68_MOVEM},
	{"MOVEP",INSTR68_MOVEP},
	{"MOVEQ",INSTR68_MOVEQ},
	{"MOVES",INSTR68_MOVES},
	{"MULS",INSTR68_MULS},
	{"MULSL",INSTR68_MULSL},
	{"MULU",INSTR68_MULU},
	{"MULUL",INSTR68_MULUL},
	{"NBCD",INSTR68_NBCD},
	{"NEG",INSTR68_NEG},
	{"NEGX",INSTR68_NEGX},
	{"NOP",INSTR68_NOP},
	{"NOT",INSTR68_NOT},
	{"OR",INSTR68_OR},
	{"ORI",INSTR68_OR},
	{"PACK",INSTR68_PACK},
	{"PEA",INSTR68_PEA},
	{"RESET",INSTR68_RESET},
	{"ROL",INSTR68_ROL},
	{"ROR",INSTR68_ROR},
	{"ROXL",INSTR68_ROXL},
	{"ROXR",INSTR68_ROXR},
	{"RTD",INSTR68_RTD},
	{"RTE",INSTR68_RTE},
	{"RTM",INSTR68_RTM},
	{"RTR",INSTR68_RTR},
	{"RTS",INSTR68_RTS},
	{"SBCD",INSTR68_SBCD},
	{"ST",INSTR68_ST},
	{"SF",INSTR68_SF},
	{"SHI",INSTR68_SHI},
	{"SLS",INSTR68_SLS},
	{"SCC",INSTR68_SCC},
	{"SCS",INSTR68_SCS},
	{"SHS",INSTR68_SCC},
	{"SLO",INSTR68_SCS},
	{"SNE",INSTR68_SNE},
	{"SEQ",INSTR68_SEQ},
	{"SVC",INSTR68_SVC},
	{"SVS",INSTR68_SVS},
	{"SPL",INSTR68_SPL},
	{"SMI",INSTR68_SMI},
	{"SGE",INSTR68_SGE},
	{"SLT",INSTR68_SLT},
	{"SGT",INSTR68_SGT},
	{"SLE",INSTR68_SLE},
	{"SUB",INSTR68_SUB},
	{"SUBA",INSTR68_SUB},
	{"SUBI",INSTR68_SUB},
	{"SUBQ",INSTR68_SUBQ},
	{"SUBX",INSTR68_SUBX},
	{"SWAP",INSTR68_SWAP},
	{"TAS",INSTR68_TAS},
	{"TST",INSTR68_TST},
	{"UNLK",INSTR68_UNLK},
	{"UNPK",INSTR68_UNPK},

	/* aliases */
	{"BHS",INSTR68_BCC},
	{"BLO",INSTR68_BCS},

	/* Makron */
	{"CLR_X",INSTR68_CLR_X},
	{"RTKF",INSTR68_RTKF},
	{"CLEARX",INSTR68_CLR_X},

	/* devpac special aliases/macros */
	{"PUSH",INSTR68_PUSH},
	{"PULL",INSTR68_PULL},

	/* direktiv vars operand ej skall parsas */

	/* data direktiv */
	{"DC",INSTR68_DC},
	{"DCB",INSTR68_DCB},
	{"DS",INSTR68_DS},
	{"EVEN",INSTR68_EVEN},
	{"CNOP",INSTR68_CNOP},
	{"INCBIN",INSTR68_INCBIN},

	/* EQU mfl "label" direktiv */
	{"EQU",INSTR68_EQU},
	{"SET",INSTR68_SET},

	/* RS direktiv */
	{"RS",INSTR68_RS},
	{"RSRESET",INSTR68_RSRESET},
	{"RSSET",INSTR68_RSSET},

	/* diverse vettiga assemblerdirektiv */
	{"INCLUDE",INSTR68_INCLUDE},
	{"IFNE",INSTR68_IFNE},
	{"ENDC",INSTR68_ENDC},
	{"REPT",INSTR68_REPT},
	{"ENDR",INSTR68_ENDR},
	{"MACRO",INSTR68_MACRO},
	{"ENDM",INSTR68_ENDM},
	{"STRUCT",INSTR68_STRUCT},
	{"ENDS",INSTR68_ENDS},

	/* knasiga assemblerdirektiv */
	{"OUTPUT",INSTR68_OUTPUT},
	{"OPT",INSTR68_OPT},
	{"SECTION",INSTR68_SECTION},

	{"",INSTR68_UNKNOWN}
};

/********************** niv† 2 ******************
* decode instruktioner
* dessa anropas till st”rsta delen ifr†n niva 1
* men „ven viss korskoppling kan f”rekomma
************************************************/

static char *getOperand(char *dst,char **srcptr)
{
	char *src=*srcptr;
	char *dststring=dst;
	int depth=0;
	int done=0;

	while(isspace(*src)) src++;	/* hoppa ”ver ev inledande whitespace */

	while(*src && !isspace(*src) && !done) {
		switch(*src) {
		case ';':
			done=1;
			break;
		case '(':
			depth++;
			break;
		case ')':
			depth--;
			break;
		case ',':
			if(depth==0) {
				done=1;
			}
			break;
		case '"':
			do {
				*dst++=*src++;
			} while(*src && *src!='"');
			if(!*src) {
				*srcptr=src;
				return NULL;
			}
			break;
		}
		if(!done)
			*dst++=*src++;
	}
	*dst='\0'; /* avslutande nolla „r bra och ha.. */

	/* lite felcheck f”r operandf„lt som „r n†got annat... */
	if(strlen(dststring)==0)	/* inget alls */
		return NULL;
/*	if(dststring[0]=='*')		/ * * kommentar * /
**		return NULL;
*/
	*srcptr=src;
	return src;
}

static char *getOperands(char *dst,char **srcptr)
{
	char *src=*srcptr;
	char *dststring=dst;
	int depth=0;
	int done=0;

	while(isspace(*src)) src++;	/* hoppa ”ver ev inledande whitespace */

	while(*src && !isspace(*src) && !done) {
		switch(*src) {
		case ';':
			done=1;
			continue; /* Avbryt while loopen */
		case '(':
			depth++;
			break;
		case ')':
			depth--;
			break;
		case ',':
			break;
		case '"':
			do {
				*dst++=*src++;
			} while(*src && *src!='"');
			if(!*src) {
				*srcptr=src;
				return NULL;
			}
			break;
		}
		*dst++=*src++;
	}
	*dst='\0'; /* avslutande nolla „r bra och ha.. */

	/* lite felcheck f”r operandf„lt som „r n†got annat... */
	if(strlen(dststring)==0)	/* inget alls */
		return NULL;
/*	if(dststring[0]=='*')		/ * * kommentar * /
**		return NULL;
*/
	*srcptr=src;
	return src;
}


/* decodeReg
** avkoda en str„ng som „r ett register
*/
static REGISTER decodeReg(char *reg)
{
	if(strlen(reg)==2) {
		switch(toupper(reg[0])) {
		case 'A':
			if(isdigit(reg[1]))
				return REG_A0+reg[1]-'0';

		case 'D':
			if(isdigit(reg[1]))
				return REG_D0+reg[1]-'0';
		}
	}
	if(stricmp(reg,"PC")==0) return REG_PC;
	if(stricmp(reg,"SP")==0) return REG_SP;
	if(stricmp(reg,"CCR")==0) return REG_CCR;
	return NULL;
}

/* decodeReg2
** avkoda f”rsta tv† tecknen av en str„ng som Rn
*/
static REGISTER decodeReg2(char *reg)
{
	if(isalnum(reg[2]))
		return REG_NONE;

	switch(toupper(reg[0])) {
	case 'A':
		if(isdigit(reg[1]))
			return REG_A0+reg[1]-'0';
	case 'D':
		if(isdigit(reg[1]))
			return REG_D0+reg[1]-'0';
	case 'P':
		if(toupper(reg[1])=='C')
			return REG_PC;
		break;
	case 'S':
		if(toupper(reg[1])=='P')
			return REG_SP;
		break;
	}
	return REG_NONE;
}

/* Kontrollera om en str„ng „r en multireg str„ng */
static char *isMultiReg(char *str)
{
	if(strlen(str)<5)
		return NULL;

	if(decodeReg2(str) && (str[2]=='-' || str[2]=='/'))
		return str;
	return NULL;
}

/* Avkoda en multireg str„ng */
static REGS decodeMultiReg(char *str)
{
	REGISTER reg,reg2;
	REGS mask;
	REGS err;

	err.mask=0;
	mask.mask=0;

	reg = decodeReg2(str);
	if(reg<REG_D0 || reg>REG_A7) {
		fprintf(stderr,"Parse error in decodeMultireg <%s>\n",str);
		exit(1);
	}

	str+=2;

	mask.mask |= REGMASKBIT(reg);
litemera:
	switch(*str++) {
	case '/':
		mask.mask |= decodeMultiReg(str).mask;
		break;
	case '-':
		reg2 = decodeReg2(str);
		if(reg2<REG_D0 || reg2>REG_A7)
			return err;
		for (; reg <= reg2; reg++)
			mask.mask |= REGMASKBIT(reg);
		str+=2;
		goto litemera;
	case '\0':
		break;
	default:
		fprintf(stderr,"Parse error in decodeMultireg <%s>\n",str);
		exit(1);
	}

	return mask;
}

/* decodeSize
** avkoda en size
*/
static SIZE decodeSize(char size)
{
	switch(toupper(size)) {
	case 'S':
	case 'B':
		return SIZE_B;
	case 'W':
		return SIZE_W;
	case 'L':
		return SIZE_L;
	default:
   	return SIZE_UNKNOWN;
	}
}


/* decodeIndexReg
** avkoda ett index register, med scale och size
** lagra resultatet i operanden
*/
static REGISTER decodeIndexReg(OPERAND *op,char *str)
{
	REGISTER reg;
	/* Rn.size*scale */

	/* Rn */
	reg=decodeReg2(str);
	if(!reg)
		return REG_NONE;
	str+=2;

	/* .size */
	if(*str=='.') {
		op->indexSize=decodeSize(str[1]);
		str+=2;
	}

	/* *scale */
	if(*str=='*') {
		op->indexScale=str[1]-'0';
		str+=2;
	}

	if(*str) {
		fprintf(stderr,"Error in parser!!!! (or parse error) while parsing index register <%s>\n",str);
		exit(1);
	}


	return reg;
}


/* getConstant(char *str)
** h„mta en konstant ifr†n en operand str„ng
*/
static char *getConstant(char **str)
{
	static char ret[256];
	int depth;
	char *dst;
	char *ptr;

	for(ptr=*str,dst=ret,depth=0;*ptr;*dst++=*ptr++) {
		if(*ptr=='(') {
			if(decodeReg2(ptr+1))
				break;
			depth++;
		} else if(*ptr==',' || *ptr=='[') {
			break;
		} else if(*ptr==')') {
			depth--;
		}
	}
	while(depth!=0) {
		if(*dst=='(') depth--;
		if(*dst==')') depth++;
		if(depth!=0) {
			dst--;
			ptr--;
		}
	}
	*dst++=0;

	*str=ptr;
	if(ret[0])
		return ret;
	else
		return NULL;
}

/* Avkoda en AM_INDIRECT operand str„ng */
static OPERAND *decodeIndirect(OPERAND *op,char *src)
{
	char *constant,*part;

	/* const(base,index*scale.size,displ) */
	op->addrMode=AM_INDIRECT;

	/* konstant */
	constant=getConstant(&src);
	if(constant)
		strcpy(op->constant.text,constant);

	/* Hoppa ”ver inledande parentesen */
	if(*src++!='(')
		return NULL;

	/* Sl„ng avslutande parentesen */
	if(*src && src[strlen(src)-1]==')')
		src[strlen(src)-1]=0;
	else
		return NULL;

	/* S†ja, nu kan vi dela upp den p† , */

	/* F”rst konstant.. */
	part=strtok(src,",");
	if(part) {
		if(decodeReg(part))
			goto baseReg;
		strcpy(op->constant.text,part);
	}

	/* F”rst bas register */
	part=strtok(NULL,",");
baseReg:
	if(part) {
		op->reg=decodeReg(part);
		if(!op->reg)
			return NULL;
	}

	/* Index register? */
	part=strtok(NULL,",");
	if(part) {
		op->index=decodeIndexReg(op,part);
		if(!op->index)
			return NULL;
	}

	/* Felcheck... */
	if(strtok(NULL,","))
		return NULL;

	/* Jaha, d† var vi klara... */
	return op;
}

/* decodeMemIndirect, avkoda en memory indirect operand */
static OPERAND *decodeMemIndirect(OPERAND *op,char *src)
{
	char *part;
	char klammer[256];
	char therest[256];

	/* const(base,index*scale.size,displ) */
	op->addrMode=AM_MEM_INDIRECT;

	/* Hoppa ”ver inledande parentesen */
	if(*src++!='(')
		return NULL;

	/* Sl„ng avslutande parentesen */
	if(*src && src[strlen(src)-1]==')')
		src[strlen(src)-1]=0;
	else
		return NULL;

	/* Hoppa ”ver inledande hitlern */
	if(*src++!='[')
		return NULL;

	/* dela upp i klammer del och therest */
	part=strtok(src,"]");
	if(!part)
		return NULL;
	strcpy(klammer,part);
	part=strtok(NULL,"");
	if(!part)
		part="";
	strcpy(therest,part);

	/* Behandla klammer delen */

	/* F”rst konstant.. */
	part=strtok(klammer,",");
	if(part) {
		if(decodeReg(part))
			goto innerBaseReg;
		strcpy(op->constant.text,part);
	}

	/* Sen bas register */
	part=strtok(NULL,",");
innerBaseReg:
	if(part) {
		op->reg=decodeReg(part);
		if(!op->reg)
			goto innerIndexReg;
	}

	/* sen ev Index register? */
	part=strtok(NULL,",");
innerIndexReg:
	if(part) {
		op->index=decodeIndexReg(op,part);
		if(!op->index)
			return NULL;
	}

	/* sist Felcheck... */
	if(strtok(NULL,","))
		return NULL;

	/* Behandla the rest */

	/* F”rst ev index */
	part=strtok(therest,",");
	if(part) {
		op->outerIndex=decodeIndexReg(op,part);
		if(!op->outerIndex)
			goto outerDispl;
	}

	/* Sen displacement */
	part=strtok(NULL,",");
outerDispl:
	if(part) {
		strcpy(op->outerDispl.text,part);
	}

	/* sist Felcheck... */
	if(strtok(NULL,","))
		return NULL;

	/* Jaha, d† var vi klara... */
	return op;
}

/* register			Dn An PC CCR SP
**	immediate		#nnn
** Abs				xxx
** Postinc			(An)+
** PreDec			-(An)
** Reg Indirect   Od1(Od2,An,Xn.size*scale)
** Mem Indirect	Od([Bd,An,Xn.size*scale],Xn.size*scale,Od)
**
*/
static OPERAND *decodeOperand(OPERAND *op,char *src)
{
	char *ptr;
	enum {
		OKANT,
		IMMEDIATE,
		REG,
		MULTIREG,
		ABSOLUTE,
		INDIRECT,
		IND_PREDEC,
		IND_POSTINC,
		MEM_INDIRECT
	} type=OKANT;

	memset(op,0,sizeof(OPERAND));

	/* F”rst f”rs”ker vi klura ut vad det kan vara... */
	if(src[0]=='#')
		type=IMMEDIATE;
	else if(strchr(src,'['))
		type=MEM_INDIRECT;
	else if(strchr(src,','))
		type=INDIRECT;
	else if(decodeReg(src))
		type=REG;
	else if(isMultiReg(src))
		type=MULTIREG;
	else { /* nu „r fr†gan, „r det abs eller indirect utan displacement/index?? */
		type=ABSOLUTE;
		for(ptr=src;*ptr;ptr++) {
			if(ptr[0]=='(' && decodeReg2(ptr+1) && ptr[3]==')') {
				op->reg=decodeReg2(ptr+1);
				if(ptr[4]=='+') {
					type=IND_POSTINC;
					break;
				} else if(ptr>src && ptr[-1]=='-') {
					type=IND_PREDEC;
					break;
				} else {
					type=INDIRECT;
					break;
				}
			}
		}
	}

	/* S†ja, nu var det dags att tolka skiten ocks† */
	switch(type) {
	case IMMEDIATE:
		op->addrMode=AM_IMMEDIATE;
		strcpy(op->constant.text,src+1);
		break;
	case ABSOLUTE:
		op->addrMode=AM_ABS_MEM;
		strcpy(op->constant.text,src);
		break;
	case REG:
		op->reg=decodeReg(src);
		op->addrMode=AM_REG;
		break;
	case MULTIREG:
		op->regs=decodeMultiReg(src);
		op->addrMode=AM_MULTI_REG;
		break;
	case IND_POSTINC:
		op->addrMode=AM_INDIRECT_POSTINC;
		break;
	case IND_PREDEC:
		op->addrMode=AM_INDIRECT_PREDEC;
		break;
	case INDIRECT:
		op->addrMode=AM_INDIRECT;
		return decodeIndirect(op,src);
	case MEM_INDIRECT:
		op->addrMode=AM_MEM_INDIRECT;
		return decodeMemIndirect(op,src);
	default:
		fprintf(stderr,"Error in PARSER.C, unexpected type %d in parseOperand\n",type);
		exit(1);
	}
	return op;
}

static INSTRCODE68 decodeInstr(char *instr)
{
	struct instrName *n;

	for(n=instrtab;n->code!=INSTR68_UNKNOWN;n++) {
		if(stricmp(n->name,instr)==0) {
			return n->code;
		}
	}
	return INSTR68_UNKNOWN;
}


/********************** niv† 1 ******************
* parser funktioner
* dessa funktioner returnerar PARSED resultat om de lyckas
* anropas i allm„nhet av parse(), men viss korskoppling kan f”rekomma
*
* prototyp:
*  PARSED *parse_<name>(void);
*
*************************************************/

static PARSED *parseError(P_ERROR err)
{
	static PARSED ret={IS_PARSE_ERROR};

	ret.data.error.error=err;
	strncpy(ret.data.error.source,state.line,SOURCE_LINE_SIZE);
	ret.data.error.lineNumber=state.lineNumber;
	ret.data.error.column=state.column;
	strncpy(ret.data.error.fileName,state.fileName,FILE_NAME_SIZE);

	state.column=state.lineLength;

	return &ret;
}

/* parseLabel
** l„s en label
*/
static PARSED *parseLabel(void)
{
	static PARSED ret={IS_LABEL};
	char *dst,*src;
/*	int i;
*/
	src=state.line;
	dst=ret.data.label.text;

/*	for(i=0;i<state.column && isspace(*src);i++,src++);
**	if(i<state.column)
*/
	if(state.column>0)
		return NULL;

	/* kopiera label */
	while(*src && !isspace(*src) && *src!=':' && *src!=';' && *src!='*') {
		*dst++=*src++;
		state.column++;
	}
	*dst++='\0';
	if(strlen(ret.data.label.text)==0)
		return NULL;

	if(*src==':') {
		state.column++;
	}

	return &ret;
}


/* parseInstruction
** l„s och tolka en instruktion
*/
static PARSED *parseInstruction(void)
{
	static PARSED ret={IS_INSTR};
	char instr[256];
	char *src,*dst;
	INSTRCODE68 instrcode;
	char operand[256];

	src=&state.line[state.column];

	/* hoppa ”ver white space i b”rjan */
	while(isspace(*src))
		src++;

	/* kopiera instruktionsnamnet fram till white '.' eller ';' */
	for(dst = instr;
		*src && *src != '.' && !isspace(*src) && *src!=';';
		*dst++ = *src++);
	*dst++='\0';	/* avslutande nolla i instruktionen */

	/* avkoda instruktionsnamnet */
	instrcode=decodeInstr(instr);
	if(instrcode==INSTR68_UNKNOWN) /* var det ej en k„nd instruktion? */
		instrcode=INSTR68_MACRO_USE; /* D† „r det ett macro */
	/* Kopiera instruktions namnet och nummer */
	strcpy(ret.data.instr.instrText.text,instr);
	ret.data.instr.instr=instrcode;

	/* ta reda p† instruktions-storleken */
	if (*src == '.')
	{
		switch(toupper(*++src))
		{
			case 'B': ret.data.instr.size=SIZE_B; break;
			case 'W': ret.data.instr.size=SIZE_W; break;
			case 'L': ret.data.instr.size=SIZE_L; break;
			case 'S': ret.data.instr.size=SIZE_B; break;
			default: {
				state.column=(int)(src-state.line);
				return parseError(P_ERROR_SIZE);
			}
		}
		src++;
	} else {
		ret.data.instr.size=SIZE_UNKNOWN;
	}

	/********** parsa eventuella operander ********/

	while(isspace(*src))
		src++;

	if (NO_PARSE_NEEDED(instrcode))
	{
		getOperands(ret.data.instr.op1.constant.text, &src);
		if(strlen(ret.data.instr.op1.constant.text)>0) {
			ret.data.instr.op1.addrMode=AM_SPECIAL;
			ret.data.instr.op2.addrMode=AM_NO_OPERAND;
		} else {
			ret.data.instr.op1.addrMode=AM_NO_OPERAND;
			ret.data.instr.op2.addrMode=AM_NO_OPERAND;
		}
	}
	else if(getOperand(operand,&src)) /* operand 1 */
	{
		if(!decodeOperand(&ret.data.instr.op1,operand))
		{
			state.column=(int)(src-state.line);
			return parseError(P_ERROR_OPERAND);
		}
		if(*src==',') /* operand 2 */
		{
			src++;
			if(getOperand(operand,&src))
			{
				if(!decodeOperand(&ret.data.instr.op2,operand))
				{
					state.column=(int)(src-state.line);
					return parseError(P_ERROR_OPERAND);
				}
			}
			else
			{
				state.column=(int)(src-state.line);
				return parseError(P_ERROR_OPERAND);
			}
		}
		else
		{
			ret.data.instr.op2.addrMode=AM_NO_OPERAND;
		}
	}
	else
	{
		ret.data.instr.op1.addrMode=AM_NO_OPERAND;
		ret.data.instr.op2.addrMode=AM_NO_OPERAND;
	}

	/* klart.. updatera state och returnera resultat */
	state.column=(int)(src-state.line);
	return &ret;
}

/* parseComment
** l„s en kommentar
*/
static PARSED *parseComment(void)
{
	static PARSED ret={IS_COMMENT};
	char *src,*chk;

	src=state.line+state.column;

	while(isspace(*src)) {
		src++;
		state.column++;
	}

	switch(*src) {
	case '*':
	case ';':
		break;
	case '\0':
		return NULL;
	default:
		state.column=(int)(src-state.line);
		return parseError(P_ERROR_JUNK);
	}

	strncpy(ret.data.comment.text,src+1,COMMENT_SIZE);
	for(chk=state.line;	chk<src && isspace(*chk); chk++);
	if(chk==src) {
		ret.type=IS_COMMENT;
	} else {
		ret.type=IS_COMMENT_EOL;
	}
	state.column=state.lineLength;
	return &ret;
}

/* parseSourceLine
** returnera information om k„llkodsrad
*/
static PARSED *parseSourceLine(void)
{
	static PARSED ret={IS_SOURCE_LINE};

	strncpy(ret.data.source.text,state.line,SOURCE_LINE_SIZE-1);
	ret.data.source.lineNumber=state.lineNumber;
	strncpy(ret.data.source.fileName,state.fileName,FILE_NAME_SIZE-1);

	return &ret;
}

/************** niv† 0 ***************************
* st”dfunktioner till de globala funktionerna
**************************************************/
static char *readLine(void)
{
	int len;
	state.lineNumber+=1;
	if(fgets(state.line,256,state.file)) {
		state.column=0;
		len=strlen(state.line);
		while(len>0 && state.line[len-1]=='\n')
			state.line[--len]='\0';
		state.lineLength=len;
		return state.line;
	} else {
		return NULL;
	}
}



/************ Globala funktioner ****************
* dessa „r de funktioner som kan anropas utifr†n
*
* bool initparser(FILE *file,char *name)
* PARSER_STATE getparserstate(void)
* bool setparserstate(PARSER_STATE *state)
*
*************************************************/


/* initparser
** initierar parsern mot en given fil
*/
int initParser(FILE *file,char *name)
{
	if(!file)
		file=fopen(name,"r");
	if(!file)
		return 0;
	state.file=file;
	state.lineNumber=0;
	state.lineLength=0;
	state.column=0;
	strcpy(state.fileName,name);

	return 1;
}

/* getParserState
** l„ser av aktuellt parserstate
** anv„nds tex n„r man tempor„rt skall hoppa till en annan fil
*/
PARSER_STATE *getParserState(PARSER_STATE *stateptr)
{
	*stateptr=state;
	 return stateptr;
}

/* setParserState
** s„tter aktuellt parserstate
** anv„nds tex n„r man hoppar tillbaks ifr†n en annan fil
*/
PARSER_STATE *setParserState(PARSER_STATE *stateptr)
{
	state=*stateptr;
	return stateptr;
}

/* parse
** detta „r huvudfunktionen.
** denna funktion anropas f”r att utf”ra sj„lva parsningen
*/
PARSED *parse(void)
{
	int c;
	PARSED *ret=NULL;
	char *src;

parseMore:
	/* dags att l„sa in mer data ? */
	if(state.column>=state.lineLength) {
		if(readLine())	/* l„s in data */
			ret=parseSourceLine();
		else
			return NULL; /* NULL om ingen mer data */
	}

	if(!ret) {
		/* hoppa ”ver ev whitespace */
		for(src=state.line+state.column;isspace(*src);src++,state.column++);

		/* parsa data */
		c=*src;
		switch(c) {
		case '*': 		/* kommentar... */
		case ';':
			ret=parseComment();
			break;
		case '\0': 		/* inge mer p† denna rad */
			goto parseMore;
		default: 		/* instruktion eller label */
			ret=parseLabel();
			if(!ret)
				ret=parseInstruction();
			if(!ret)
				parseError(P_ERROR_INSTRUKTION);
			break;
		}
	}
	if(!ret)
		goto parseMore;
	ret->lineNumber=state.lineNumber;
	return ret;
}
