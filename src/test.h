#ifndef TEST_H
#define TEST_H

#include "m682386.h"
#include "parser.h"
#include "exp68000.h"
#include "flaggopt.h"
#include "x_is_c.h"
#include "match86.h"

void printParsed(PARSED *);
void printExp68(EXP68000 *,int data,int flags);
void printFlaggopt(FLAGGOPT *,int data,int flags);
void printXisC(X_IS_C *,int data,int flags);
void printMatch86(MATCH86 *,int data,int flags);
void initTest(FILE *);

#endif /* TEST_H */