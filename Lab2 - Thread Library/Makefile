#
# file:    Makefile
#
CFLAGS = -g -Wall -pedantic

LDLIBS = -lcheck -lsubunit -lm -lrt

all: test

# this uses the following default build rules:
# .c to .o: $(CC) $(CFLAGS) file.c -c -o file.o
# multiple .o to exe. : $(CC) $(LDFLAGS) file.o [file.o..] $(LDLIBS) -o exe

test: test.o qthread.o

clean:
	rm -f *.o test 
