FLAGS = -Wall -g 
FLAGS2 = -lpthread
CC = gcc
INV = invert
OBJS = ppmtools.o invert.o timeprofiler.o

all: ${INV} clean

clean:
	rm *o

${INV}:	${OBJS}
	${CC} ${FLAGS} ${OBJS} ${FLAGS2} -o $@	
	
.c.o:
	${CC} ${FLAGS} $< -c
	
	
# # # # # # # # #

ppmtools.o: ppmtools.c
invert.o: invert.c
timeprofiler.o: timeprofiler.c

