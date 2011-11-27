CC=gcc
PROG_NAME=NekoGroup
INCS=
SRCS=core.c command.c log.c debug.c main.c
OBJS=${SRCS:.c=.o}
LIBS=

CFLAGS= -Wall -O2 `pkg-config --cflags glib-2.0 purple`
LDFLAGS= -Wall -O2 `pkg-config --libs glib-2.0 purple`

all: ${PROG_NAME}

${PROG_NAME}:${OBJS}
	${CC} -o ${PROG_NAME} ${OBJS} ${LDFLAGS} ${LIBS}

${OBJS}:${INCS}

.c.o:
	${CC} -c $< ${CFLAGS}

clean:
	rm -f *.o ${PROG_NAME}

rebuild: clean all
