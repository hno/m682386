#include <stdio.h>
#include <stdlib.h>

#include "dotlabel.h"
#include "exp68000.h"
#include "fcomment.h"
#include "flaggopt.h"
#include "generate.h"
#include "i80386.h"
#include "info386.h"
#include "m68000.h"
#include "m682386.h"
#include "match86.h"
#include "multireg.h"
#include "numbers.h"
#include "opmodes.h"
#include "parser.h"
#include "regass.h"
#include "simple68.h"
#include "test.h"
#include "x_is_c.h"

void main(void)
{
	printf("Source line:%d\n",sizeof(SOURCE_LINE));
   printf("INSTRTEXT  :%d\n",sizeof(INSTRTEXT));
	printf("PARSEDINSTR:%d\n",sizeof(PARSEDINSTR68));
	printf("INSTR      :%d\n",sizeof(INSTR68));
	printf("CONSTANT   :%d\n",sizeof(CONSTANT));
	printf("PARSED     :%d\n",sizeof(PARSED));
   exit(0);
}