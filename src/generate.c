/* Generera 386 k„llkod */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "m682386.h"
#include "info386.h"
#include "parser.h"
#include "exp68000.h"
#include "flaggopt.h"
#include "x_is_c.h"
#include "match86.h"
#include "regass.h"
#include "generate.h"

#define READER regAss
typedef REGASS INPUT;

/* Some statistics... */
static long sourceLines=0;
static long instruktions=0;
static long labels=0;
static long errors=0;
static long comments=0;

/* eol comments */
static COMMENT eolComment;

/* there is a label on this line */
int labelOnLine;

/* Where to send output */
static FILE *out;

/* Ouput flags */
static GENOPTIONS opt;

/* last input line, used on errors */
static SOURCE_LINE sourceLine;

static char *getInstr86Text(INSTRCODE86 instr)
{
	INFO86 *info;
	static char temp[40];

	info=info386(instr);
	if(!info) {
		sprintf(temp,"UNIMPL%d",instr);
		return temp;
	}

	return info->text;
}

static char *getInstrText(INSTR86 instr)
{
	static char temp[40];

	if(instr.instr==INSTR86_MACRO_USE) {
		strcpy(temp,instr.instrText.text);
		return temp;
	}

   return getInstr86Text(instr.instr);
}

static char *regAssErrorDescr[]={
	"NONE",
	"REG_TEMP used but not set",
	"Unknown operand mode",
	"Unknown addrmode",
	"Invalid register usage",
	"Invalid size for instruction or operand"
};

static char *parseErrorDescr[]={
	"NONE",
	"INSTRUCTION",
	"JUNK IN LINE",
	"OPERAND",
	"INVALID SIZE"
};

static char *exp68ErrorDescr[]={
	"NONE",
	"INSTRUCTION MISSING IN TABLE",
	"UNKNOWN STRUCT TYPE FROM FLAGGOPT",
	"WRONG NUMBER OF ARGUMENTS",
	"UNKNOWN OPSMODE",
	"MISSING SIZE"
};

static char *flaggOptErrorDescr[]={
	"NONE",
	"USED FLAGS NEVER SET"
};

static char *opsModeDescr[]={
	"O_UNKNOWN",
	"O_SPECIAL",
	"O_NONE",
	"O_NORMAL",
	"O_SRC",
	"O_DST",
	"O_MOD",
	"O_MOD2",
	"O_MDST",
	"O_MDST1",
	"O_FLOW",
	"O_FLOW2",
	"O_CHECK",
	"O_DATA",
	"O_MACRO",
};

static char *opModeDescr[]={
	"OP_NONE",
	"OP_NO_OPERAND",
	"OP_READ",
	"OP_WRITE",
	"OP_MODIFY",
	"OP_FLOW",
	"OP_DATA",
	"OP_MACRO",
};


static char *match86ErrorDescr[]={
	"NONE",
	"INSTRUCTION UNIMPLEMENTED",
	"FLAGS NOT SET",
	"FLAGS NOT USED",
	"FLAGS NOT PRESERVED",
	"NO INFO ABOUT 386 INSTR",
	"68 AND 386 OPMODE DIFFER",
	"INVALID SIZE"
};

static char *XisCErrorDescr[]={
	"OK",
	"C modified while X in use"
};


static struct {
	REGISTER reg;
	char normal[30],byte[30],word[30],dword[30];
} regTable[]={
	{REG_A0,"A0","A0.B","A0.W","A0.L"},
	{REG_A1,"A1","A1.B","A1.W","A1.L"},
	{REG_A2,"A2","A2.B","A2.W","A2.L"},
	{REG_A3,"A3","A3.B","A3.W","A3.L"},
	{REG_A4,"A4","A4.B","A4.W","A4.L"},
	{REG_A5,"A5","A5.B","A5.W","A5.L"},
	{REG_A6,"A6","A6.B","A6.W","A6.L"},
	{REG_A7,"%A7%","%A7.B%","%A7.W%","%A7.L%"},
	{REG_D0,"D0","D0.B","D0.W","D0.L"},
	{REG_D1,"D1","D1.B","D1.W","D1.L"},
	{REG_D2,"D2","D2.B","D2.W","D2.L"},
	{REG_D3,"D3","D3.B","D3.W","D3.L"},
	{REG_D4,"D4","D4.B","D4.W","D4.L"},
	{REG_D5,"D5","D5.B","D5.W","D5.L"},
	{REG_D6,"D6","D6.B","D6.W","D6.L"},
	{REG_D7,"D7","D7.B","D7.W","D7.L"},
	{REG_EAX,"EAX","AL","AX","EAX"},
	{REG_EBX,"EBX","BL","BX","EBX"},
	{REG_ECX,"ECX","CL","CX","ECX"},
	{REG_EDX,"EDX","DL","DX","EDX"},
	{REG_ESI,"ESI","%ESI.B%","SI","ESI"},
	{REG_EDI,"EDI","%EDI.B%","DI","EDI"},
	{REG_EBP,"EBP","%EBP.B%","BP","EBP"},
	{REG_ESP,"ESP","%ESP.B%","%ESP.W%","ESP"},
	{REG_TEMP,"%TEMP%","%TEMP.B%","%TEMP.W%","%TEMP.L%"},
	{REG_TEMP_INDEX,"%TEMPI%","%TEMPI.B%","%TEMPI.W%","%TEMPI.L%"},
	{REG_IP,"EIP","%EIP.B%","%EIP.W%","EIP"},
	{REG_NONE,"%???%","%???.B%","%???.W%","%???.L%"},
};

static char *getReg(REGISTER reg, SIZE size)
{
	int i;
	static char hubba[256];

	for(i=0;regTable[i].reg!=reg && regTable[i].reg!=REG_NONE;i++);
	switch(size) {
	case SIZE_NONE:
		return regTable[i].normal;
	case SIZE_B:
		return regTable[i].byte;
	case SIZE_W:
		return regTable[i].word;
	case SIZE_L:
		return regTable[i].dword;
	default:
		errors+=1;
		sprintf(hubba,"%%%s.ERROR_SIZE%%",regTable[i].normal);
		return hubba;
	}
}

static char *getOpSize(SIZE size)
{
	switch(size) {
	case SIZE_B:
		return ".B";
	case SIZE_W:
		return ".W";
	case SIZE_L:
		return ".L";
	case SIZE_NONE:
		return "";
	default:
		errors+=1;
		return "%ERROR_SIZE%";
	}
}

static char *segOverride(OPERAND op)
{
	switch(op.addrMode) {
	case AM_INDIRECT:
		switch(op.reg) {
		case REG_EBP:
			return "DS:";
		case REG_ESP:
			return "SS:";
		case REG_NONE:
			switch(op.index) {
			case REG_EBP:
				return "DS:";
			case REG_ESP:
				return "SS:";
			default:
				break;
			}
			break;
		default:
			break;
		}
	default:
		break;
	}
	return "";
}


static int scatprintf(char *str,char *fmt,...)
{
	int ret;
	va_list ap;
	va_start(ap,fmt);
	ret=vsprintf(str+strlen(str),fmt,ap);
	va_end(ap);
	return ret;
}

static int eprintf(FILE *out,char *fmt,...)
{
	int ret=0;
	va_list ap;
	va_start(ap,fmt);
	if(!opt.noErrors)
		ret=vfprintf(out,fmt,ap);
	if(out!=stdout)
		vfprintf(stderr,fmt,ap);
	va_end(ap);
	return ret;
}

static char *getOp(OPERAND op)
{
	static char out[256];
	/* OP_FLOW on AM_ABS_MEM should not use brakets */
	switch(op.addrMode) {
	case AM_REG:
		sprintf(out,"%s",getReg(op.reg,op.size));
		break;
	case AM_INDIRECT:
		sprintf(out,"%s%s[",segOverride(op),op.constant.text);
		if(op.reg!=REG_NONE) {
			scatprintf(out,"%s",getReg(op.reg,SIZE_L));
		}
		if(op.index!=REG_NONE) {
			if(op.reg!=REG_NONE)
				scatprintf(out,"+");
			scatprintf(out,"%s",getReg(op.index,SIZE_L));
			if(op.indexScale>1)
				scatprintf(out,"*%d",op.indexScale);
		}
		scatprintf(out,"]%s",getOpSize(op.size));
		break;
	case AM_ABS_MEM:
		switch(op.mode) {
		case OP_FLOW: /* FLOW does not use size or brakets  */
			sprintf(out,"%s",op.constant.text);
			break;
		default:
			sprintf(out,"[%s]%s",op.constant.text,getOpSize(op.size));
			break;
		}
		break;
	case AM_IMMEDIATE:
		sprintf(out,"offset %s",op.constant.text);
		break;
	case AM_SPECIAL:
		sprintf(out,"%s",op.constant.text);
		break;
	default:
		errors+=1;
		sprintf(out,"GEN_ERROR_ADDRMODE_%d",op.addrMode);
		break;
	}
	return out;
}

static void genInstr(INSTR86 instr)
{
	int isRs=0;
	/* Lite specal special grejer */
	switch(instr.instr) {
	case INSTR86_RSTRUCB:
	case INSTR86_RSTRUCW:
	case INSTR86_RSTRUCD:
		isRs=1;
		if(labelOnLine && !opt.rsAsData) {
			fprintf(out,"\t%s RSCOUNTER",getInstr86Text(INSTR86_EQU));
			if(*eolComment.text)
				fprintf(out," ; %s\n",eolComment.text);
			else
				fprintf(out,"\n");
			*eolComment.text=0;
		}
		break;
	default:
		break;
	}

	if(opt.rsAsData && isRs) {
		switch(instr.instr) {
		case INSTR86_RSTRUCB:
			instr.instr=INSTR86_DB;
			break;
		case INSTR86_RSTRUCW:
			instr.instr=INSTR86_DW;
			break;
		case INSTR86_RSTRUCD:
			instr.instr=INSTR86_DD;
			break;
		default:
			break;
		}
		if(strcmp(instr.op1.constant.text,"0")!=0) {
			fprintf(out,"\t%-4s %s %s (?)"
					,getInstrText(instr),getOp(instr.op1)
					,getInstr86Text(INSTR86_DUP));
		} else {
			switch(instr.instr) {
			case INSTR86_DB:
				fprintf(out,"\tLABEL BYTE");
				break;
			case INSTR86_DW:
				fprintf(out,"\tLABEL WORD");
				break;
			case INSTR86_DD:
				fprintf(out,"\tLABEL DWORD");
				break;
			default:
				break;
			}
		}
	}
	else {
		/* Generera instruktion */
		fprintf(out,"\t%-4s",getInstrText(instr));

		if(instr.op1.addrMode)
			fprintf(out," %s",getOp(instr.op1));
		if(instr.op2.addrMode)
			fprintf(out,",%s",getOp(instr.op2));
	}
	if(*eolComment.text)
		fprintf(out," ; %s",eolComment.text);
	*eolComment.text='\0';
	fprintf(out,"\n");
}

int generate(void)
{
	INPUT *in;
	LABEL label;

	labelOnLine=0;

	in=READER();


processIt:
	if(!in) {
		eprintf(out,";TOTAL: IN %ld, INSTR %ld, LABELS %ld, COMMENTS %ld, ERRORS %ld\n",
				sourceLines,instruktions,labels,comments,errors);
		return 0;
	}

	switch(in->type) {
	case IS_PARSE_ERROR:
		errors+=1;
		{
			PARSED *p=(PARSED *)in;
			char *ptr;
			for(ptr=p->data.error.source;*ptr;ptr++)
				if(isspace(*ptr)) *ptr=' ';
			eprintf(out,";PARSE_ERROR: file:%s line:%d column:%d error:%s\n;%s\n;%*s\n",
					p->data.error.fileName,
					p->data.error.lineNumber,p->data.error.column,
					parseErrorDescr[p->data.error.error],
					p->data.error.source,
					p->data.error.column+1,"^");
		}
		break;
	case IS_EXP68_ERROR:
		errors+=1;
		if(opt.sourceOnError)
			eprintf(out,";%s\n",sourceLine.text);
		{
			EXP68000 *e=(EXP68000 *)in;
			switch(e->data.error.error) {
			case E68_E_NARGS:
				eprintf(out,";EXP68000_ERROR: line:%d error:%s for opsMode %s\n"
						,e->lineNumber,exp68ErrorDescr[e->data.error.error]
						,e->data.error.nargs
						,opsModeDescr[e->data.error.opsMode]);
				eprintf(out,"\n");
				break;
			case E68_E_OPSMODE:
				eprintf(out,";EXP68000_ERROR: line:%d error:%s %d\n"
						,e->lineNumber,exp68ErrorDescr[e->data.error.error]
						,e->data.error.opsMode);
				break;
			default:
				eprintf(out,";EXP68000_ERROR: line:%d error:%s\n",
						e->lineNumber,exp68ErrorDescr[e->data.error.error]);
				break;
			}
		}
		break;
	case IS_X_IS_C_ERROR:
		errors+=1;
		if(opt.sourceOnError)
			eprintf(out,";%s\n",sourceLine.text);
		{
			X_IS_C *i=(X_IS_C *)in;
			eprintf(out,";X_IS_C ERROR: line:%d error:%s!!!\n"
					,i->lineNumber,XisCErrorDescr[i->data.error]);
		}
		break;
	case IS_FLAGGOPT_ERROR:
		errors+=1;
		{
			FLAGGOPT *i=(FLAGGOPT *)in;
			eprintf(out,";FLAGGOPT ERROR: line:%d error:%s (%x)!!!\n"
					,i->lineNumber,flaggOptErrorDescr[i->data.error.error]
					,i->data.error.flags);
		}
		break;
	case IS_MATCH86_ERROR:
		errors+=1;
		if(opt.sourceOnError)
			eprintf(out,";%s\n",sourceLine.text);
		{
			MATCH86 *m=(MATCH86 *)in;
			INFO86 *info;
			switch(m->data.error.error) {
			case M86_E_UNIMPL:
				eprintf(out,";MATCH86_ERROR: line:%d error:%s %s (%d)\n"
						,m->lineNumber,match86ErrorDescr[m->data.error.error]
						,m->data.error.instrText.text
						,m->data.error.instr68);
				break;
			case M86_E_INFO86:
				info=info386(m->data.error.instr86);
				if(info) {
					eprintf(out,";MATCH86_ERROR: line:%d error:%s %s (%s)\n"
							,m->lineNumber,match86ErrorDescr[m->data.error.error]
							,info->text,m->data.error.instrText.text);
				} else {
					eprintf(out,";MATCH86_ERROR: line:%d error:%s %d (%s)\n"
							,m->lineNumber,match86ErrorDescr[m->data.error.error]
							,m->data.error.instr86,m->data.error.instrText.text);
				}
				break;
			case M86_E_FLAGS_SET:			/* Flaggor s„tts inte som de borde */
			case M86_E_FLAGS_USED:		/* Flaggor anv„nds inte som de borde */
			case M86_E_FLAGS_PRESERVED:	/* Flaggor bevaras inte som de borde */
				eprintf(out,";MATCH86_ERROR: line:%d error:%s (%x)\n"
						,m->lineNumber,match86ErrorDescr[m->data.error.error]
						,m->data.error.flags);
				break;
			default:
				eprintf(out,";MATCH86_ERROR: line:%d error:%s\n"
						,m->lineNumber,match86ErrorDescr[m->data.error.error]);
				break;
			}
		}
		break;
	case IS_REGASS_ERROR:
		errors+=1;
		if(opt.sourceOnError)
			eprintf(out,";%s\n",sourceLine.text);
		{
			REGASS *r=(REGASS *)in;
			switch(r->data.error.error) {
			case R_E_TMPUSE:
				eprintf(out,";REGASS_WARNING: %s\n"
						,regAssErrorDescr[r->data.error.error]);
				break;
			case R_E_ADDRMODE:
				eprintf(out,";REGASS_ERROR: %s %d\n"
						,regAssErrorDescr[r->data.error.error]
						,r->data.error.addrMode);
				break;
			case R_E_OPMODE:
				eprintf(out,";REGASS_ERROR: %s %s\n"
						,regAssErrorDescr[r->data.error.error]
						,opModeDescr[r->data.error.opMode]);
				break;
			default:
				if(r->lineNumber)
					eprintf(out,";REGASS_ERROR: line:%d error:%s\n"
							,r->lineNumber,regAssErrorDescr[r->data.error.error]);
				else
					eprintf(out,";REGASS_ERROR: error:%s\n"
							,regAssErrorDescr[r->data.error.error]);
				break;
			}
		}
		break;
	case IS_SOURCE_LINE:
		sourceLines+=1;
		sourceLine=in->data.source;
		if(opt.m68Source) {
			if(*eolComment.text)
				fprintf(out,"; %-30s",eolComment.text);
			else
				fprintf(out,"  %-30s","");
			*eolComment.text='\0';
			fprintf(out,";LINE %d: %s\n",in->lineNumber,in->data.source.text);
		}
		/* Beh†ll tomrader */
		if(!*in->data.source.text)
			fprintf(out,"\n");
		break;
	case IS_LABEL:
		labelOnLine=0;
		labels+=1;
		label=in->data.label;
		in=READER();
		if(in) switch(in->type) {
		case IS_INSTR:
			switch(in->data.instr.opsMode) {
			case O_DATA:
				fprintf(out,"%s",label.text);
				break;
			default:
				fprintf(out,"%s:",label.text);
				break;
			}
			labelOnLine=1;
			break;
		default:
			fprintf(out,"%s:",label.text);
			if(*eolComment.text)
				fprintf(out," ; %s\n",eolComment.text);
			else
				fprintf(out,"\n");
			*eolComment.text='\0';
			break;
		}
		goto processIt;
	case IS_INSTR:
		instruktions+=1;
		genInstr(in->data.instr);
		break;
	case IS_COMMENT:
		comments+=1;
		fprintf(out,";%s\n",in->data.comment.text);
		break;
	case IS_COMMENT_EOL:
		comments+=1;
		if(*eolComment.text)
			fprintf(out,"; %s\n",eolComment.text);
		strcpy(eolComment.text,in->data.comment.text);
		break;
	default:
		if(opt.sourceOnError)
			eprintf(out,";%s\n",sourceLine.text);
		eprintf(out,";GENERATE_ERROR: Unknown type %d\n",in->type);
		break;
	}
	return 1;
}

void initGenerate(FILE *output,GENOPTIONS options)
{
	out=output;
	opt=options;
	*eolComment.text=0;
}
