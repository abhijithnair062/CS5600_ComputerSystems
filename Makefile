#
# Makefile for lab 1
#

CFLAGS = -ggdb3 -Wall -pedantic -g -fstack-protector-all -fsanitize=address
shell56: shell56.c parser.c commandProcessor.c
	gcc -fsanitize=address shell56.c parser.c commandProcessor.c -g -o shell56 $(CFLAGS)

clean:
	rm -f *.o shell56
