#ifndef REGASS_H
#define REGASS_H

/* Include fil till regAss */
#include "m682386.h"

/* Felkoder till regass */
typedef enum {
	R_E_OK=0,
	R_E_TMPUSE,		/* REG_TEMP anv„nds utan att den „r satt */
	R_E_OPMODE,		/* Unknown operand mode */
	R_E_ADDRMODE,	/* Unknown addr mode */
	R_E_REGUSE,		/* Invalid register usage */
	R_E_SIZE,		/* Invalid size for instr/register */
} RASS_ERRORCODE;

/* Error struktur till regass */
typedef struct {
	RASS_ERRORCODE error;
	OPMODE opMode;
	ADDRMODE addrMode;
	REGISTER reg;
	SIZE size;
} RASS_ERROR;

/* utstruktur fr†n regAss */
typedef struct {
	STRUCT_TYPE type;
	LINENUMBER lineNumber;
	union {
		INSTR86 instr;
		LABEL label;
		COMMENT comment;
		SOURCE_LINE source;
		RASS_ERROR error;
	} data;
	FLAGMASK flagsSet,flagsUsed,flagsPreserved;	/* 80386 flaggor */
} REGASS;


REGASS *regAss(void);

#endif