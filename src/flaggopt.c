/* flaggopt
** clean up unused flag info
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "flaggopt.h"

/******************************/
/* our locaion in the processing chain */
#include "opmodes.h"
#define READER opModes
typedef OPMODES INPUT;
typedef FLAGGOPT OUTPUT;
/******************************/

#define MAX_WORK_ARRAY 130  /* Maximum window size to process */

/* processing window/area */
static OUTPUT work[MAX_WORK_ARRAY];
static int realelem=0; 
static int nelem=0;
static int currelem=0;
static FLAGMASK flagsNotSet=0; /* flags not yet set */
static int flagsNotSetLine;	/* where the flags are destroyed */

/* get data up to next instruction modifying all flags  */
static void getdata(void)
{
	INPUT *in;

	/* move remaining elements */
	for(nelem=0;nelem<realelem-currelem;nelem++)
		work[nelem]=work[currelem+nelem];

	currelem=0;
	realelem=nelem;

	/* read in more elements */
	do {
		in=READER();
		if(!in)
			goto done;
		*(INPUT *)&work[realelem]=*in;
		realelem++;
		if(realelem>=MAX_WORK_ARRAY) {
			fprintf(stderr,"Out of work space in flaggopt at line %d\n",
					in->lineNumber);
			exit(1);
		}
	} while((in->flagsSet|(F_ALL & ~in->flagsPreserved))!=F_ALL);

	if(realelem>1)
		nelem=realelem-1;
	else
done:
		nelem=realelem;
}

/* doflaggopt: remove unused flags */
static void doflaggopt(void)
{
	int pos;
	FLAGMASK used=0;
	getdata();
	if(!nelem)
		return;

	/* No flags needed after the block */
	if(nelem<realelem)
		/* In case something which sets all flags
		** should need some flags as input
		*/
		used=work[nelem].flagsUsed;
	else
		used=0;

	/* scan the current block bottom-up starting with the last instruction */
	for(pos=nelem-1;pos>=0;pos--) {
		/* Flags not needed are not interesting */
		work[pos].flagsSet &= used;

		/* Flags being set does not need to be set earlier */
		used &= ~work[pos].flagsSet;

		/* Flags used but not preserved are odd.. */
		if(used & ~work[pos].flagsPreserved) {
			flagsNotSet|=used & ~work[pos].flagsPreserved;
			flagsNotSetLine=work[pos].lineNumber;
		}

		/* The search only needs to continue for those
		** flags which are preserved.
		*/
		used &= work[pos].flagsPreserved;

		/* Only flags needed are interestning to keep
		*/
		work[pos].flagsPreserved &= used;

		/* Flags used are needed */
		used |= work[pos].flagsUsed;
	}
}

OUTPUT errorFlagsNotSet(FLAGMASK flags)
{
	OUTPUT out;
	memset(&out,0,sizeof(out));
	out.type=IS_FLAGGOPT_ERROR;
	out.lineNumber=flagsNotSetLine;
	out.data.error.error=FOPT_E_FLAGSNOTSET;
	out.data.error.flags=flags;
	return out;
}

/* Send output to the next step */
OUTPUT *flaggopt(void)
{
	/* generate more elements */
	if(currelem>=nelem) {
		doflaggopt();
	}

	/* warn about flags we failed to find the source for */
	if(flagsNotSet) {
		static OUTPUT ret;
		ret=errorFlagsNotSet(flagsNotSet);
		flagsNotSet=0;
		return &ret;
	}

	/* EOF */
	if(currelem>=nelem)
		return NULL;

	return &work[currelem++];
}
