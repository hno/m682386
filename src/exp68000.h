#ifndef EXP68000_H
#define EXP68000_H
/* definitions for exp68000
**
** exp68000 expands 68000 instructions with flag info
*/

#ifndef M682386_H
#include "m682386.h"
#endif

typedef enum {
	E68_E_OK,
	E68_E_INSTR,
	E68_E_TYPE,
	E68_E_NARGS,
	E68_E_OPSMODE,
	E68_E_SIZE
} E68_ERRORCODE;

typedef struct {
	E68_ERRORCODE error;
	int nargs;
	OPSMODE opsMode;
} E68_ERROR;

/* expanderad instruktion */
typedef struct {
	STRUCT_TYPE type;
	LINENUMBER lineNumber;
	union {
		INSTR68 instr;
		LABEL label;
		COMMENT comment;
		SOURCE_LINE source;
		E68_ERROR error;
	} data;
	FLAGMASK flagsSet,flagsUsed,flagsPreserved;
} EXP68000;

EXP68000 *exp68000(void);
#endif /* EXP68000_H */
