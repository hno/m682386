#ifndef FLAGGOPT_H
#define FLAGGOPT_H

#ifndef EXP68000_h
#include "exp68000.h"
#endif

typedef enum {
	FOPT_E_OK,
	FOPT_E_FLAGSNOTSET
} FLAGGOPT_ERRORCODE;

typedef struct {
	FLAGGOPT_ERRORCODE error;
	FLAGMASK flags;
} FLAGGOPT_ERROR;

/* flag optimized instruction */
typedef struct {
	STRUCT_TYPE type;
	LINENUMBER lineNumber;
	union {
		INSTR68 instr;
		LABEL label;
		COMMENT comment;
		SOURCE_LINE source;
		FLAGGOPT_ERROR error;
	} data;
	FLAGMASK flagsSet,flagsUsed,flagsPreserved;
} FLAGGOPT;


FLAGGOPT *flaggopt(void);

#endif /* FLAGGOPT_H */
