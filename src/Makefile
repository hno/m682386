all: 682386

#CFLAGS = -O2 -Wall
CFLAGS = 
OBJS =  \
	dotlabel.o exp68000.o fcomment.o flaggopt.o generate.o info386.o \
	main.o match86.o memind.o multireg.o numbers.o opmodes.o \
	parser.o regass.o simple68.o test.o x_is_c.o 

682386: $(OBJS)
	$(CC) -lm $(OBJS)

clean:
	rm -f 682386 $(OBJS)
