#ifndef INFO386_H
#define INFO386_H

#ifndef M682386_H
#include "m682386.h"
#endif /* M682386_H */

typedef struct {
	char text[30];
	INSTRCODE86 instr;
	FLAGMASK flagsSet;
	FLAGMASK flagsUsed;
	FLAGMASK flagsPreserved;
	OPSMODE opsMode;
} INFO86;

#ifdef F_ALL
#undef F_ALL
#endif F_ALL
#define F_ALL (F_N | F_Z | F_V | F_C)

INFO86 *info386(INSTRCODE86);

#endif /* INFO386_H */
