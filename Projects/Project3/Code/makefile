FLAGS = -Wall -g 
FLAGS2 = -lpthread -lrt -D_REENTRANT
CC = gcc
GRY = Main
OBJS = pixel2ascii.o timeprofiler.o Main.o

all: ${GRY} clean

clean:
	rm *o

${GRY}:	${OBJS}
	${CC} ${FLAGS} ${OBJS} ${FLAGS2} -o $@	
	
.c.o:
	${CC} ${FLAGS} $< -c
	
	
# # # # # # # # #

pixel2ascii.o: pixel2ascii.c
timeprofiler.o: timeprofiler.c
Main.o: Main.c

