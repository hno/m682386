/* Fixa kommentarer
** flytta EOL kommentarer till f”re ev instruktion
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "fcomment.h"

#define MAX_OUTPUT 3

/* Varifr†n f†r vi v†r input? */
#define READER exp68000
typedef EXP68000 INPUT;
typedef FCOMMENT OUTPUT;

#define MAX_ACTIVE 3

/* Flytta EOL kommentarer */

static OUTPUT *fixComment1(void)
{
	INPUT in,in2;
	INPUT *p1,*p2;
	static INPUT ret;
	static INPUT last;
	static int lastfinns=0;

	if(lastfinns) {
		in=last;
		lastfinns=0;
	} else {
		p1=READER();
		if(!p1) return NULL;
		in=*p1;
	}

	/* Standard retur v„rdet */
	ret=in;

	p2=READER();
	if(!p2) {
		return &ret;
	}
	in2=*p2;

	/* standard n„sta „r i samma ordning som fr†n b”rjan */
	last=in2;
	lastfinns=1;

	if(in2.type==IS_COMMENT_EOL) {
		switch(in.type) {
		/* Det „r en EOL kommentar som skall f”rflyttas ... */
		case IS_INSTR:
			/* returnera in2, och sparar in f”r framtiden */
			last=in;
			ret=in2;
			break;
		/* Kommentaren skulle inte f”rflyttas. */
		default:
			break;
		}
	}

	return &ret;
}

#undef READER
#define READER fixComment1

OUTPUT *fixComment(void)
{
	INPUT in,in2;
	INPUT *p1,*p2;
	static INPUT ret;
	static INPUT last;
	static int lastfinns=0;

	if(lastfinns) {
		in=last;
		lastfinns=0;
	} else {
		p1=READER();
		if(!p1) return NULL;
		in=*p1;
	}

	/* Standard retur v„rdet */
	ret=in;

	p2=READER();
	if(!p2) {
		return &ret;
	}
	in2=*p2;

	/* standard n„sta „r i samma ordning som fr†n b”rjan */
	last=in2;
	lastfinns=1;

	if(in2.type==IS_COMMENT_EOL) {
		switch(in.type) {
		/* Det „r en EOL kommentar som skall f”rflyttas ... */
		case IS_LABEL:
			/* returnera in2, och sparar in f”r framtiden */
			last=in;
			ret=in2;
			break;
		/* Kommentaren skulle inte f”rflyttas. */
		default:
			break;
		}
	}

	return &ret;
}

