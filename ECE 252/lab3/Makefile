# Makefile, ECE252  
# Yiqing Huang, 2018/11/02

CC = gcc 
CFLAGS = -Wall -std=c99 -std=gnu99 -g # "curl-config --cflags" output is empty  
LD = gcc
LDFLAGS = -std=c99 -g -std=gnu99
LDLIBS = -lcurl -lz -lpthread # "curl-config --libs" output 

SRCS   = paster2.c
OBJS1  = paster2.o
TARGETS= paster2

all: ${TARGETS}

main_simple.out: $(OBJS1) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o *.png -r *strips $(TARGETS)
