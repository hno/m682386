#ifndef GENRATE_H
#define GENERATE_H

#include "stdio.h"

typedef struct {
	int m68Source:1		/* Include 68000 source */
		,sourceOnError:1	/* Include 68000 source on errors */
		,noErrors:1			/* Generate no errors in output */
		,rsAsData:1;		/* Generate rs as data directives */
} GENOPTIONS;

int generate(void);
void initGenerate(FILE *out,GENOPTIONS options);

#endif
