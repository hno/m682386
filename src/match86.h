#ifndef MATCH86_H
#define MATCH86_H

/* Includefil till match86 */

#ifndef M682386_H
#include "m682386.h"
#endif


/* felkoder vid matchning mot 80386 instruktion */
typedef enum {
	M86_E_OK,					/* Inget fel... */
	M86_E_UNIMPL,				/* 68000 instruktionen ej implementerad */
	M86_E_FLAGS_SET,			/* Flaggor s„tts inte som de borde */
	M86_E_FLAGS_USED,			/* Flaggor anv„nds inte som de borde */
	M86_E_FLAGS_PRESERVED,	/* Flaggor bevaras inte som de borde */
	M86_E_INFO86,				/* Det saknas information om 80386 instruktionen i i80386.c */
	M86_E_OPMODE,				/* 68k och 386 instruktionen hanterar operanderna olika */
	M86_E_SIZE					/* Ok„nd/Ej till†ten storlek */
} M86_ERRORCODE;

/* struktur som returneras vid fel */
typedef struct {
	M86_ERRORCODE error;
	INSTRCODE68 instr68;	/* Den 68000 instruktion som genererade felet */
	INSTRCODE86 instr86;	/* Den 80386 instruktion som genererade felet */
	INSTRTEXT instrText; /* Instruktion i textformat */
	FLAGMASK flags;		/* De flaggor felet g„ller */
	SIZE size;				/* storlek (E_SIZE) */
} M86_ERROR;

/* utstruktur fr†n match86 */
typedef struct {
	STRUCT_TYPE type;
	LINENUMBER lineNumber;
	union {
		INSTR86 instr;
		LABEL label;
		COMMENT comment;
		SOURCE_LINE source;
		M86_ERROR error;
	} data;
	FLAGMASK flagsSet,flagsUsed,flagsPreserved;	/* 80386 flaggor */
} MATCH86;

/* prototyp f”r match86 */
MATCH86 *match86(void);

#endif /* MATCH86_H */