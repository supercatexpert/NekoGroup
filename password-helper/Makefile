CC=cc
PROG_NAME=NekoGroupPasswordHelper
SOURCES=ng-password-helper.c
HEADERS=ng-password-helper.h
PKGS=glib-2.0 libsecret-1

SRCS=${SOURCES}
OBJS=${SRCS:.c=.o}
LIBS=
INCS=${HEADERS}

CFLAGS= -Wall -O2 `pkg-config --cflags ${PKGS}`
LDFLAGS= -Wall -O2 `pkg-config --libs ${PKGS}`

all: ${PROG_NAME}

${PROG_NAME}:${OBJS}
	${CC} -o ${PROG_NAME} ${OBJS} ${LDFLAGS} ${LIBS} 

${OBJS}:${INCS} 

.c.o:${SRCS}
	${CC} -c $< ${CFLAGS}

clean:
	rm -f *.o *.gch ${PROG_NAME}

rebuild: clean all
