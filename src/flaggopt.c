/* flaggopt
** rensa bort lite onodig flagginfo.....
**
**
** l„s tills n†gon beh”ver flaggor, s”k tillbaks till vem som
** beh”vde och radera bort all annan flagginfo
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "flaggopt.h"

/******************************/
/* Varifr†n f†r vi v†r input? */
#include "opmodes.h"
#define READER opModes
typedef OPMODES INPUT;
/* Vad har vi f”r output? */
typedef FLAGGOPT OUTPUT;
/******************************/

#define MAX_WORK_ARRAY 130  /* Max element inom ett arbetsomrade som vi kan hantera */

/* arbets array */
static OUTPUT work[MAX_WORK_ARRAY];
static int realelem=0; /* verkligt antal element i arrayen */
static int nelem=0; /* antal element som arbetas p† i arrayen */
static int currelem=0; /* aktuellt element i arrayen */
static FLAGMASK flagsNotSet=0; /* Flaggor som ej „r satta f”r blocket */
static int flagsNotSetLine;	/* Vid vilken rad flaggorna f”rst”rs */
/* h„mta data tills n„sta instruktion som p†verkar alla flaggor */
static void getdata(void)
{
	INPUT *in;

	/* flytta kvarvarande antal element */
	for(nelem=0;nelem<realelem-currelem;nelem++)
		work[nelem]=work[currelem+nelem];

	/* fixa currelem och realelem efter nya f”rh†llanden */
	currelem=0;
	realelem=nelem;

	/* l„s in nya element */
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

	/* klart.. s„tt antal element i arrayen */
	if(realelem>1)
		nelem=realelem-1;
	else
done:
		nelem=realelem;
}

/* doflaggopt: optimera bort onodiga flaggor */
static void doflaggopt(void)
{
	int pos;
	FLAGMASK used=0;
	getdata();
	if(!nelem)
		return;

	/* Inga flaggor behovs efter blocket */
	if(nelem<realelem)
		/* Utifall att n†got som p†verkar alla flaggor
		** skulle beh”va n†gra flaggor
		*/
		used=work[nelem].flagsUsed;
	else
		used=0;

	for(pos=nelem-1;pos>=0;pos--) {
		/* Flaggor som inte beh”vs „r inte intressanta */
		work[pos].flagsSet &= used;

		/* De flaggor som s„tts beh”vs inte s„ttas tidigare*/
		used &= ~work[pos].flagsSet;

		/* De flaggor som beh”vs och inte bibeh†lls „r skumma */
		if(used & ~work[pos].flagsPreserved) {
			flagsNotSet|=used & ~work[pos].flagsPreserved;
			flagsNotSetLine=work[pos].lineNumber;
		}

		/* Endast de flaggor som bibeh†lls av denna instruktion
		** „r n†gon ide att leta vidare efter
		*/
		used &= work[pos].flagsPreserved;

		/* Endast de flaggor som fortfarande beh”vs „r n†gon ide
		** att bibeh†lla
		*/
		work[pos].flagsPreserved &= used;

		/* Flaggor som beh”vs beh”vs.... */
		used |= work[pos].flagsUsed;
	}
}

/* Generera en error struktur */
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

/* H„mta upp n„sta element ur arrayen, „r det tomt generera nya */
OUTPUT *flaggopt(void)
{
	/* generera nytt om det beh”vs */
	if(currelem>=nelem) {
		doflaggopt();
	}

	/* Generera varning f”r osatta flaggor */
	if(flagsNotSet) {
		static OUTPUT ret;
		ret=errorFlagsNotSet(flagsNotSet);
		flagsNotSet=0;
		return &ret;
	}

	/* vid EOF returnera NULL */
	if(currelem>=nelem)
		return NULL;

	/* returnera aktuellt element */
	return &work[currelem++];
}
