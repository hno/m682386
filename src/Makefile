all: 682386

CFLAGS = -Wall -Werror -g
LDFLAGS = $(CFLAGS)
LDLIBS = -lm

OBJS =  \
	dotlabel.o exp68000.o fcomment.o flaggopt.o generate.o info386.o \
	main.o match86.o memind.o multireg.o numbers.o opmodes.o \
	parser.o regass.o simple68.o test.o x_is_c.o 

682386: $(OBJS)
	$(LINK.o) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

clean:
	rm -f 682386 $(OBJS)
