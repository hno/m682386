#ifndef PARSER_H
#define PARSER_H

#ifndef __STDIO_H
#include <stdio.h>
#endif

#ifndef M682386_H
#include "M682386.h"
#endif /* 682386_H */

#define PARSE_ERROR_SIZE    (120)

typedef enum {
	P_ERROR_NONE=0,
	P_ERROR_INSTRUKTION,
	P_ERROR_JUNK,
	P_ERROR_OPERAND,
	P_ERROR_SIZE
} P_ERROR;

typedef struct {
	FILE *file;
	char line[256];
	int lineNumber;
	int column;
	int lineLength;
	char fileName[256];
} PARSER_STATE;

#define NO_PARSE_NEEDED(a) (a > NO_PARSE_NEEDED_START && a < NO_PARSE_NEEDED_END || a == INSTR68_MACRO_USE)

typedef struct {
	char source[SOURCE_LINE_SIZE];
	P_ERROR error;
	unsigned int lineNumber;
	unsigned int column;
	char fileName[FILE_NAME_SIZE];
} PARSE_ERROR;

typedef struct {
	INSTRCODE68 instr;
   INSTRTEXT instrText;
	SIZE size;
	OPERAND op1,op2;
} PARSEDINSTR68;

typedef struct {
	STRUCT_TYPE type;
	unsigned int lineNumber;
	union {
		PARSEDINSTR68 instr;
		LABEL label;
		COMMENT comment;
		PARSE_ERROR error;
		SOURCE_LINE source;
	} data;
} PARSED;

/*************************************************************/

/*************************************************************/
int initParser(FILE *file,char *fileName);
PARSER_STATE *getParserState(PARSER_STATE *);
PARSER_STATE *setParserState(PARSER_STATE *);
PARSED *parse(void);

#endif /* PARSER_H */
