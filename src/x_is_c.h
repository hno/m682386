#ifndef X_IS_C_H
#define X_IS_C_H
/* Headerfil till x_is_c */

#include "m682386.h"

typedef enum {
	XC_E_OK,
	XC_E_CMODIFIED	/* B†de X och C anv„nds */
} X_IS_C_ERROR;

/* Instruktion med X likst„llt med C */
typedef struct {
	STRUCT_TYPE type;
	LINENUMBER lineNumber;
	union {
		INSTR68 instr;
		LABEL label;
		COMMENT comment;
		SOURCE_LINE source;
		X_IS_C_ERROR error;
	} data;
	FLAGMASK flagsSet,flagsUsed,flagsPreserved;
} X_IS_C;

X_IS_C *XisC(void);
#endif /* X_IS_C_H */
