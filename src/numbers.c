/* Fixa kommentarer
** flytta EOL kommentarer till f”re ev instruktion
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "m682386.h"
#include "numbers.h"

#define MAX_OUTPUT 3

/* Varifr†n f†r vi v†r input? */
#include "fcomment.h"
#define READER fixComment
typedef FCOMMENT INPUT;
typedef NUMBERS OUTPUT;

/* Fixa nummer representation */
static void fixNumbers(char *str)
{
	char tmp[256];
	char *ptr;
	char *src;

	memset(tmp,0,sizeof(tmp));
	for(ptr=tmp,src=str;*src;*ptr++=*src++) {
		switch(*src) {
		case '%':
			src++;
			*ptr++='0';
			while(isxdigit(*src))
				*ptr++=*src++;
			*ptr++='B';
			break;
		case '$':
			src++;
			*ptr++='0';
			while(isxdigit(*src))
				*ptr++=*src++;
			*ptr++='H';
			break;
		default:
			break;
		}
	}
	*ptr='\0';
	strcpy(str,tmp);
}

/* Jobba p† lite */
OUTPUT *numbers(void)
{
	INPUT *in;

	in=READER();

	switch(in->type) {
	case IS_INSTR:
		fixNumbers(in->data.instr.op1.constant.text);
		fixNumbers(in->data.instr.op1.constant.text);
		break;
	default:
		break;
	}

	return (OUTPUT *)in;
}

