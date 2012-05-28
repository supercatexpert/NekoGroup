CC=gcc
PROG_NAME=NekoGroup
SOURCES=ng-core.c ng-db.c ng-cmd.c ng-config.c ng-bot.c ng-main.c main.c
HEADERS=ng-core.h ng-db.h ng-cmd.h ng-config.h ng-bot.h ng-main.h ng-common.h
PKGS=glib-2.0 gobject-2.0 loudmouth-1.0 libmongo-client

SRCS=${SOURCES} ng-marshal.c
OBJS=${SRCS:.c=.o}
LIBS=
INCS=${HEADERS} ng-marshal.h

CFLAGS= -Wall -O2 `pkg-config --cflags ${PKGS}`
LDFLAGS= -Wall -O2 `pkg-config --libs ${PKGS}`

all: ${PROG_NAME}

${PROG_NAME}:${OBJS}
	${CC} -o ${PROG_NAME} ${OBJS} ${LDFLAGS} ${LIBS} 

${OBJS}:${INCS} 

.c.o:${SRCS}
	${CC} -c $< ${CFLAGS}

clean:
	rm -f *.o ${PROG_NAME} ng-marshal.c ng-marshal.h

rebuild: clean all

ng-marshal.c: ng-marshal.list
	glib-genmarshal --prefix=ng_marshal ng-marshal.list \
        --body > ng-marshal.c

ng-marshal.h: ng-marshal.list
	glib-genmarshal --prefix=ng_marshal ng-marshal.list \
        --header > ng-marshal.h
        
