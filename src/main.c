/* huvudprogram till 682386.... */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m682386.h"
#include "parser.h"
#include "exp68000.h"
#include "fcomment.h"
#include "dotlabel.h"
#include "opmodes.h"
#include "flaggopt.h"
#include "x_is_c.h"
#include "match86.h"
#include "generate.h"
#include "test.h"

#define TEST_DATA					1
#define TEST_FLAGS				1
#define TEST_PARSER 				0
#define TEST_EXP68000			0
#define TEST_FCOMMENT			0
#define TEST_DOTLABEL			0
#define TEST_OPMODES				0
#define TEST_FLAGGOPT			0
#define TEST_X_IS_C				0
#define TEST_MATCH86				0
#define GENERATE					1

extern unsigned _stklen=10000;

/* objectName
** ett namn som anv„nds f”r automat generarade labels, och annat
** d„r en identifierare beh”vs
*/
char objectName[256];

/* objectPath
** s”kv„g till fil som konverteras
*/
char objectPath[256];

int main(int argc,char **argv)
{
#if TEST_PARSER
	PARSED *gen;
#endif
#if TEST_EXP68000
	EXP68000 *gen;
#endif
#if TEST_FCOMMENT
	FCOMMENT *gen;
#endif
#if TEST_DOTLABEL
	DOTLABEL *gen;
#endif
#if TEST_OPMODES
	OPMODES *gen;
#endif
#if TEST_FLAGGOPT
	FLAGGOPT *gen;
#endif
#if TEST_X_IS_C
	X_IS_C *gen;
#endif
#if TEST_MATCH86
	MATCH86 *gen;
#endif
	GENOPTIONS genOptions;
	FILE *input=stdin;
	char *infilename="stdin";
	FILE *output=stdout;
	char *outfilename="stdout";
	char *name=NULL;

	memset(&genOptions,0,sizeof(genOptions));

	for(;argc>1 && argv[1][0]=='-';argc--,argv++) {
		switch(argv[1][1]) {
		case '?':
			printf(	"Motorola MC68000 to intel 80386 converter v1.0\n"
						"(C) 1992 Frontline Design, Henrik\n"
						"usage: %s -6er input output\n"
						"-6	Include 68000 source in output\n"
						"-e	Show 68000 source on errors\n"
						"-r	Generate rs as data directives (db,dw,dd)\n"
						,argv[0]);
			exit(0);
			break;
		case '6':
			genOptions.m68Source=1;
			break;
		case 'e':
			genOptions.sourceOnError=1;
			break;
		case 'E':
			genOptions.noErrors=1;
			break;
		case 'r':
			genOptions.rsAsData=1;
			break;
		default:
			fprintf(stderr,"Invalid option %c\n",argv[1][1]);
			break;
		}
	}

	if(argc>1) {
		infilename=argv[1];
		input=fopen(infilename,"r");
	}
	if(!input) {
		fprintf(stderr,"Can't open input file %s\n",infilename);
		exit(2);
	}

	if(argc>2) {
		outfilename=argv[2];
		output=fopen(outfilename,"w");
	}
	if(!output) {
		fprintf(stderr,"Can't open output file %s\n",outfilename);
		exit(2);
	}

	if(argc>3) {
		name=argv[3];
	}
	if(!name) {
		name=outfilename;
	}

	strcpy(objectName,outfilename);
	for(name=objectName;*name;name++)
		if(*name=='.')
			*name='\0';

	strcpy(objectPath,objectName);
	if(strrchr(objectPath,'\\'))
		strrchr(objectPath,'\\')[1]=0;

	initTest(output);
	initGenerate(output,genOptions);

	if(!initParser(input,infilename)) {
		printf("Can't init parser!!\n");
		exit(1);
	}

	fprintf(stderr,"Working from %s to %s....\n",infilename,outfilename);

#if TEST_PARSER
	while((gen=parse())!=NULL) {
		printParsed(gen);
	}
#endif

#if TEST_EXP68000
	while((gen=exp68000())!=NULL) {
		printExp68(gen,TEST_DATA,TEST_FLAGS);
	}
#endif

#if TEST_FCOMMENT
	while((gen=fixComment())!=NULL) {
		printExp68(gen,TEST_DATA,TEST_FLAGS);
	}
#endif

#if TEST_DOTLABEL
	while((gen=dotLabel())!=NULL) {
		printExp68(gen,TEST_DATA,TEST_FLAGS);
	}
#endif

#if TEST_OPMODES
	while((gen=opModes())!=NULL) {
		printExp68(gen,TEST_DATA,TEST_FLAGS);
	}
#endif

#if TEST_FLAGGOPT
	while((gen=flaggopt())!=NULL) {
		printFlaggopt(gen,TEST_DATA,TEST_FLAGS);
	}
#endif

#if TEST_X_IS_C
	while((gen=XisC())!=NULL) {
		printXisC(gen,TEST_DATA,TEST_FLAGS);
	}
#endif

#if TEST_MATCH86
	while((gen=match86())!=NULL) {
		printMatch86(gen,TEST_DATA,TEST_FLAGS);
	}
#endif

#if GENERATE
	while(generate());
#endif

	fprintf(stderr,"Done!!!\n");

	if(input!=stdin)
		fclose(input);
	if(output!=stdout)
		fclose(output);
	return 0;
}
