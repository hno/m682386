/* 
 * local labels, and operators like | & << >>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "m682386.h"
#include "exp68000.h"
#include "dotlabel.h"

#include "numbers.h"
#define READER numbers
typedef NUMBERS INPUT;
typedef DOTLABEL OUTPUT;

/* En idiot initiering, bara utifall att.... (den funkar.) */
static char fullLabel[256]="local_label_with_no_parent";

static void fixup(char *str,int macro)
{
	char *part,*ptr;
	char tmp[256];

	if(strchr(str,'.')) {
		strcpy(tmp,str);
		str[0]=0;
		for(ptr=tmp;*ptr;ptr+=strcspn(ptr,".")) {
			if(*ptr=='.') ptr++;
			if((ptr!=tmp && ptr!=tmp+1 && !isalnum(ptr[-2])) || (ptr==tmp+1)) {
				strcat(str,fullLabel);
				strcat(str,"@@");
			} else {
				if(ptr!=tmp) {
					if(macro) {
						if(strlen(ptr)==1) switch(ptr[0]) {
						case 'm':
						case 'c':
							strcat(str,".");
							break;
						default:
							strcat(str,"@");
							break;
						} else strcat(str,"@");
					} else strcat(str,"@");
				}
			}
			strncat(str,ptr,strcspn(ptr,"."));
		}
	}

	/* Replace * with $ where needed */
	for(part=strchr(str,'*');part!=NULL;part=strchr(part+1,'*')) {
		if(part==str || ispunct(part[-1]) || ispunct(part[1]))
			part[0]='$';
	}


	strcpy(tmp,str);
	str[0]=0;
#define SPECIAL_CHARS "|&<>~"
	for(part=strtok(tmp,",");part;part=strtok((char *)NULL,",")) {
		/* Fixa | */
		if(part!=tmp)
			strcat(str,",");
		if(strcspn(part,SPECIAL_CHARS)<strlen(part)) { /* Hittat en ... */
			char tmpstr[256];
			strcpy(tmpstr,part);
			part=tmpstr;
			if(macro)
				strcat(str,"<");
			for(ptr=part;*ptr;ptr=ptr+strcspn(ptr,SPECIAL_CHARS)) {
				switch(ptr[0]) {
				case '|':
					strcat(str," OR ");
					ptr+=1;
					break;
				case '&':
					strcat(str," AND ");
					ptr+=1;
					break;
				case '~':
					strcat(str," ( NOT ");
					strcat(ptr,")");
					ptr+=1;
					break;
				case '<':
					if(ptr[1]=='<') {
						strcat(str," SHL ");
						ptr+=2;
					} else {
						if(!macro || ptr!=part)
							strcat(str,"<");
						ptr+=1;
						break;
					}
					break;
				case '>':
					if(ptr[1]=='>') {
						strcat(str," SHR ");
						ptr+=2;
					} else {
						if(!macro || ptr[1])
							strcat(str,">");
						ptr+=1;
						break;
					}
					break;
				}
				strncat(str,ptr,strcspn(ptr,"|&<>"));
			}
			if(macro)
				strcat(str,">");
		} else strcat(str,part);
	}
}

/* Handle dot labels */
OUTPUT *dotLabel(void)
{
	INPUT *in;

	in=READER();
	if(!in) return NULL;

	switch(in->type) {
	case IS_LABEL:
		switch(in->data.label.text[0]) {
		case '.':
			fixup(in->data.label.text,0);
			break;
		case '_': /* Ignore underscore labels */
			break;
		default:	/* This is a full label */
			fixup(in->data.label.text,0);
			strcpy(fullLabel,in->data.label.text);
			break;
		}
		break;
	case IS_INSTR:
		if(in->data.instr.opsMode==O_SPECIAL)
			break;
		/* Search for label usage */
		/* Operand 1 */
		fixup(in->data.instr.op1.constant.text,in->data.instr.op1.mode==OP_MACRO);
		/* Operand 2 */
		fixup(in->data.instr.op2.constant.text,in->data.instr.op2.mode==OP_MACRO);
		break;
	default:
		break;
	}
	return in;
}
