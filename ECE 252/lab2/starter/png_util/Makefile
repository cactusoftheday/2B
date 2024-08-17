# Makefile, ECE252  
# Yiqing Huang

CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students 
LIB_UTIL = zutil.o crc.o
SRCS   = main.c crc.c zutil.c pnginfo.c
MAIN_OBJS = main.o $(LIB_UTIL)
PNGINFO_OBJS = pnginfo.o $(LIB_UTIL)

TARGETS= main.out pnginfo.out catpng

all: ${TARGETS}

main.out: $(MAIN_OBJS) 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

pnginfo.out: $(PNGINFO_OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

catpng: catpng.c zutil.c crc.c lab_png.h
	$(CC) $(CFLAGS) -o catpng catpng.c -lm -lz

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) 
