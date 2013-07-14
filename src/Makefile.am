bin_PROGRAMS = NekoGroup
noinst_HEADERS = 


NekoGroup_builtsources = ng-marshal.c
NekoGroup_builtheaders = ng-marshal.c

NekoGroup_SOURCES = \
    ng-core.c ng-db.c ng-cmd.c ng-config.c ng-bot.c ng-main.c \
    ng-utils.c main.c ng-marshal.c

ng-marshal.c: ng-marshal.list
	@GLIB_GENMARSHAL@ --prefix=ng_marshal $(top_srcdir)/src/ng-marshal.list \
        --body > bg-marshal.c

ng-marshal.h: ng-marshal.list
	@GLIB_GENMARSHAL@ --prefix=ng_marshal $(top_srcdir)/src/ng-marshal.list \
        --header > ng-marshal.h

NekoGroupincludedir=$(includedir)/nekogroup
NekoGroupinclude_HEADERS = ng-core.h ng-db.h ng-cmd.h ng-config.h ng-bot.h \
    ng-main.h ng-utils.h ng-common.h ng-marshal.h

NekoGroup_LDADD = @LIBOBJS@ @GLIB2_LIBS@ @LOUDMOUTH_LIBS@ @MONGOCLIENT_LIBS@

CLEANFILES = $(BUILT_SOURCES)

NekoGroup_DEPENDENCIES = @LIBOBJS@ 
NekoGroup_LDFLAGS = -fPIC -O2 -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

AM_CFLAGS = @GLIB2_CFLAGS@ @LOUDMOUTH_CFLAGS@ @MONGOCLIENT_CFLAGS@ \
    -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"$(libdir)\" \
    -DPREFIXDIR=\"$(prefix)\"
    
NekoGroup_CFLAGS = $(AM_CFLAGS)

if DEBUG_MODE
    NekoGroup_CFLAGS += -DDEBUG_MODE=1 -g
endif
    
if PLATFORM_WIN32
    NekoGroup_LDADD += -lkernel32
    NekoGroup_LDFLAGS += -mwindows
endif 

EXTRA_DIST = \
    ng-marshal.list
    
SUBDIRS = 
    

