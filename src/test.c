#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "exp68000.h"
#include "flaggopt.h"
#include "x_is_c.h"
#include "match86.h"

static FILE *out;

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
	"UNKNOWN STRUCT TYPE FROM FLAGGOPT"
};

static char *match86ErrorDescr[]={
	"NONE",
	"INSTRUCTION UNIMPLEMENTED",
	"FLAGS NOT SET",
	"FLAGS NOT USED",
	"FLAGS NOT PRESERVED",
	"NO INFO ABOUT 386 INSTR",
	"68 AND 386 OPMODE DIFFER"
};

static void printFlagsSUP(FLAGMASK s,FLAGMASK u,FLAGMASK p)
{
	if(s|u|p) {
		fprintf(out,"\t");

		if(s)
			fprintf(out,"SET:%02o ",s);
		else
			fprintf(out,"       ");

		if(u)
			fprintf(out,"USED:%02o ",u);
		else
			fprintf(out,"        ");

		if(p)
			fprintf(out,"PRESERV:%02o ",p);
		else
			fprintf(out,"           ");

		fprintf(out,"\n");
	}
}

static void printOperand(OPERAND *operand)
{
	switch(operand->addrMode)
	{
		case AM_IMMEDIATE:
			fprintf(out,"IMM:%s",operand->constant.text);
			break;
		case AM_REG:
			fprintf(out,"REG:%o",operand->reg);
			break;
		case AM_INDIRECT:
			fprintf(out,"INDIRECT: REG:%o",operand->reg);
			if(operand->index!=REG_NONE)
				fprintf(out," INDEX:%o.%d*%d",
					operand->index,operand->indexSize,operand->indexScale);
			if(*operand->constant.text)
				fprintf(out," DISP:%s",operand->constant.text);
			break;
		case AM_MEM_INDIRECT:
			fprintf(out,"MEMINDIRECT: REG:%o",operand->reg);
			if(operand->index!=REG_NONE)
				fprintf(out," INDEX:%o.%d*%d",
					operand->index,operand->indexSize,operand->indexScale);
			if(*operand->constant.text)
				fprintf(out," DISP:%s",operand->constant.text);
			if(operand->outerIndex)
				fprintf(out," OUTERINDEX:%o.%d*%d",
					operand->outerIndex,operand->indexSize,operand->indexScale);
			if(*operand->outerDispl.text)
				fprintf(out," OUTERDISP:%s",operand->outerDispl.text);
			break;
		case AM_INDIRECT_POSTINC:
			fprintf(out," POSTINC: REG:%o",operand->reg);
			break;
		case AM_INDIRECT_PREDEC:
			fprintf(out," PREDEC: REG:%o",operand->reg);
			break;
		case AM_ABS_MEM:
			fprintf(out," ABS:%s",operand->constant.text);
			break;
		case AM_MULTI_REG:
			fprintf(out," MULTIREG:%04x",operand->regs.mask);
			break;
		case AM_SPECIAL:
			fprintf(out," SPECIAL:%s",operand->constant.text);
			break;
		case AM_UNKNOWN:
			fprintf(out," UNKNOWN:%s",operand->constant.text);
			break;
		default:
			fprintf(out,"AM: PARSE ERROR AM %d!",operand->addrMode);
			break;
	}
}

void printParsed(PARSED *p)
{
	char *tmp;
	switch(p->type) {
	case IS_LABEL:
		fprintf(out,"LABEL:%s\n",p->data.label.text);
		break;
	case IS_INSTR:
		fprintf(out,"INSTR:%-3d  SIZE:%-2d\n",
		p->data.instr.instr,p->data.instr.size);
		if (p->data.instr.op1.addrMode != AM_NO_OPERAND)
		{
			fprintf(out,"\tOP1 ");
			printOperand(&p->data.instr.op1);
			fprintf(out,"\n");
		}
		if (p->data.instr.op2.addrMode != AM_NO_OPERAND)
		{
			fprintf(out,"\tOP2 ");
			printOperand(&p->data.instr.op2);
			fprintf(out,"\n");
		}
		break;
	case IS_COMMENT:
		fprintf(out,"COMMENT:%s\n",p->data.comment.text);
		break;
	case IS_COMMENT_EOL:
		fprintf(out,"COMMENT_EOL:%s\n",p->data.comment.text);
		break;
	case IS_PARSE_ERROR:
		for(tmp=p->data.error.source;*tmp;tmp++)
			if(*tmp=='\t') *tmp=' ';
		fprintf(out,"PARSE_ERROR: file:%s line:%d column:%d error:%s\n%s\n%*s\n",
			p->data.error.fileName,
			p->data.error.lineNumber,p->data.error.column,
			parseErrorDescr[p->data.error.error],
			p->data.error.source,
			p->data.error.column+1,"^");
		break;
	case IS_SOURCE_LINE:
		fprintf(out,"LINE %d:%s\n",p->data.source.lineNumber,
					p->data.source.text);
		break;
	default:
		fprintf(out,"HELP ME!!! parse() bugged\n"
			"Unknown parse type:%d\n",p->type);
		break;
	}
}

void printExp68(EXP68000 *e,int data,int flags)
{
	switch(e->type) {
	case IS_EXP68_ERROR:
		fprintf(out,"EXP68000_ERROR: line:%d error:%s\n",
			e->lineNumber,exp68ErrorDescr[e->data.error.error]);
		break;
	case IS_PARSE_ERROR:
		printParsed((PARSED *)e);
		break;
	default:
		if(data)
			printParsed((PARSED *)e);
		break;
	}
	if(flags) {
		printFlagsSUP(e->flagsSet,e->flagsUsed,e->flagsPreserved);
	}
}

void printFlaggopt(FLAGGOPT *f,int data,int flags)
{
	printExp68((EXP68000 *)f,data,flags);
}

static char *XisCError[]={
	"OK",
	"C modified while X in use"
};

void printXisC(X_IS_C *i,int data,int flags)
{
	switch(i->type) {
	case IS_X_IS_C_ERROR:
		fprintf(out,"X_IS_C ERROR: line:%d error:%s!!!\n"
			,i->lineNumber,XisCError[i->data.error]);
		if(out!=stdout)
			fprintf(stderr,"X_IS_C ERROR: line:%d error:%s!!!\n"
				,i->lineNumber,XisCError[i->data.error]);
		break;
	default:
		printFlaggopt((FLAGGOPT *)i,data,flags);
		break;
	}
}

void printMatch86(MATCH86 *m,int data,int flags)
{
	switch(m->type) {
	case IS_MATCH86_ERROR:
		fprintf(out,"MATCH86_ERROR: line:%d error:%s\n"
			,m->lineNumber,match86ErrorDescr[m->data.error.error]);
		if(out!=stdout)
			fprintf(stderr,"MATCH86_ERROR: line:%d error:%s\n"
				,m->lineNumber,match86ErrorDescr[m->data.error.error]);
		break;
	case IS_EXP68_ERROR:
	case IS_PARSE_ERROR:
	case IS_X_IS_C_ERROR:
		printXisC((X_IS_C *)m,data,0);
		break;
	default:
		if(data)
		printExp68((EXP68000 *)m,data,0);
		break;
	}
	if(flags) printFlagsSUP(m->flagsSet,m->flagsUsed,m->flagsPreserved);
}


void initTest(FILE *outfile)
{
	out=outfile;
}
