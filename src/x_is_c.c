/* X is C
** ers„tter X flaggan med C flaggan, eftersom 80386 inte har
** n†gon X flagga, utan denna funktion sk”ts enbart med C flaggan
*/

#include <string.h>
#include <stdio.h>

#include "flaggopt.h"
#include "x_is_c.h"

/* Max antal element som kan genereras per element in */
#define MAX_ELEMS	3


/* Vad f†r vi v†r input fr†n ? */
#define READER	flaggopt
typedef FLAGGOPT INPUT;


static X_IS_C work[MAX_ELEMS];
static int nelem=0;
static int currelem=0;

static void output(X_IS_C *out)
{
	if(nelem<MAX_ELEMS)
		work[nelem++]=*out;
	else
		fprintf(stderr,"Out of work space in x_is_c on line %d!!\n"
				,out->lineNumber);
}

static void error(INPUT *erl)
{
	X_IS_C out;
	memset(&out,0,sizeof(out));
	out.lineNumber=erl->lineNumber;
	out.flagsPreserved=erl->flagsPreserved;
	out.type=IS_X_IS_C_ERROR;
	out.data.error=XC_E_CMODIFIED;	/* C P†verkad medans X anv„nds */
	output(&out);
}

static void dosomemore(void)
{
	INPUT *in;
	X_IS_C out;
	in=READER();
	if(in) {
		/* Kopiera all info */
		*(INPUT *)&out=*in;

		/* Beh”ver vi jobba?, dvs beh”vs X flaggan */
		if((in->flagsSet | in->flagsUsed | in->flagsPreserved)&F_X) {

			/* F”rst lite felkontroll s† att inte C p†verkas medans X beh”vs
			** dvs att X m†ste beh†llas
			*/
			if((in->flagsSet&F_C) && (in->flagsPreserved&F_X))
				error(in);

			/* Fixa satta flaggor */
			if(in->flagsSet & F_X) {
				out.flagsSet &= ~F_X;
				out.flagsSet |= F_C;
			}

			/* Fixa anv„nda flaggor */
			if(in->flagsUsed & F_X) {
				out.flagsUsed &= ~F_X;
				out.flagsUsed |= F_C;
			}

			/* Fixa of”r„ndrade flaggor */
			if(in->flagsPreserved & F_X) {
				out.flagsPreserved &= ~F_X;
				out.flagsPreserved |= F_C;
			}
		}
		output(&out);
	}
}


X_IS_C *XisC(void)
{
	if(currelem>=nelem) {
		currelem=0;
		nelem=0;
		dosomemore();
	}

	if(currelem<nelem)
		return &work[currelem++];
	else
		return NULL;
}

