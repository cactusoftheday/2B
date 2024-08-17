# Makefile, ECE252  
# Yiqing Huang, 2018/11/02

CC = gcc 
LD = gcc
CFLAGS_COMMON = -Wall -std=gnu99 -g -DDEBUG1_
CFLAGS_XML2 = $(shell xml2-config --cflags)
CFLAGS_CURL = $(shell pkg-config --libs curl-config --cflags)
CFLAGS = $(CFLAGS_COMMON) $(CFLAGS_XML2) $(CFLAGS_CURL)

LDFLAGS = -std=c99 -g -std=gnu99
LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell pkg-config --libs curl-config --libs)
LDLIBS = $(LDLIBS_XML2) $(LDLIBS_CURL) -lcurl -lz 

LIB_UTIL = 
SRCS   = findpng3.c
OBJS1  = findpng3.o
TARGETS= findpng3

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
	rm -f *~ *.d *.o *.png *.html *.txt $(TARGETS)
