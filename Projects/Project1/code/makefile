FLAGS	= -Wall -g
CC		= gcc
PROG	= master
OBJS	= master.o

all:	${PROG}

clean:
	rm ${OBJS} ${PROG} *~
  
${PROG}:	${OBJS}
	${CC} ${FLAGS} ${OBJS} -o $@

.c.o:
	${CC} ${FLAGS} $< -c

##########################

master.o: master.c

master: master.o
