#########################################################################
# ECE252 Lab Makefile
# I. Huang, J. Parekh
#########################################################################

CC=gcc
CFLAGS=-Wall -g
LD=gcc
LDFLAGS=-g

SRCS=findpng-fix2.c catpng.c
find_OBJS=findpng-fix.o
cat_OBJS=catpng.o

all: findpng-fix2 catpng

findpng-fix2: $(find_OBJS) 
	$(LD) $(LDFLAGS) -o $@ $^

catpng: $(cat_OBJS) 
	$(LD) $(LDFLAGS) -o $@ $^ -lz

.c.o:
	$(CC) $(CFLAGS) -c $<

%.d: %.c
	gcc -MM -MF $@ $<  

-include $(SRCS:.c=.d) 

.PHONY: clean
clean:
	rm -f *.d *.o *.out catpng findpng-fix
