/* Fixup comments by moving EOL comments to the line
 * ahead of the instruction
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "fcomment.h"

#define MAX_OUTPUT 3

/* Our location in the processing chain */
#define READER exp68000
typedef EXP68000 INPUT;
typedef FCOMMENT OUTPUT;

#define MAX_ACTIVE 3

/* Move EOL comments. Look-ahead one step */

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

	ret=in;

	p2=READER();
	if(!p2) {
		return &ret;
	}
	in2=*p2;

	/* default is to keep the order */
	last=in2;
	lastfinns=1;

	if(in2.type==IS_COMMENT_EOL) {
		switch(in.type) {
		case IS_INSTR:
			/* this comment needs to be moved */
			last=in;
			ret=in2;
			break;
		default:
			/* no change needed */
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

	ret=in;

	p2=READER();
	if(!p2) {
		return &ret;
	}
	in2=*p2;

	/* default to the same order */
	last=in2;
	lastfinns=1;

	if(in2.type==IS_COMMENT_EOL) {
		switch(in.type) {
		case IS_LABEL:
			/* move comment */
			last=in;
			ret=in2;
			break;
		default:
			/* no change */
			break;
		}
	}

	return &ret;
}

