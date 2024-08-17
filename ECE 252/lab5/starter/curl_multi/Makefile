# Makefile, ECE252  
# Yiqing Huang

CC = gcc 
CFLAGS_LIBS = $(shell pkg-config --cflags libxml-2.0 -lcurl)
CFLAGS = -Wall $(CFLAGS_LIBS) -std=gnu99 -g -lcurl
LD = gcc
LDFLAGS = -std=gnu99 -g -lcurl
LDLIBS = $(shell pkg-config --libs libxml-2.0 -lcurl)

SRCS   = curl_multi_test.c
OBJS1  = curl_multi_test.o
TARGETS= curl_multi_test.out 

all: ${TARGETS}

curl_multi_test.out: $(OBJS1) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -lc -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) 
