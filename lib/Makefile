#
# Makefile
#
# See the README file for copyright information and how to reach the author.
#

include ../Make.config

LIBTARGET = libhorchi
HLIB      = -L. -lhorchi

TEST = tst

LIBOBJS = common.o db.o dbdict.o

BASELIBS = -lrt -lz -luuid -lpthread
BASELIBS += $(shell mysql_config --libs_r)

BASELIBS += $(shell xml2-config --libs)

LIBOBJS += curl.o
BASELIBS += -lcurl

DEBUG = 1

ifdef DEBUG
  CFLAGS += -ggdb -O0
endif

CFLAGS += -pthread -fPIC -Wreturn-type -Wall -Wno-parentheses -Wformat -pedantic -Wunused-variable -Wunused-label \
          -Wunused-value -Wunused-function \
          -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

CFLAGS += $(shell mysql_config --include)
CFLAGS += $(shell xml2-config --cflags)

DEFINES = $(USES)

all: lib $(TEST)
lib: $(LIBTARGET).a

$(LIBTARGET).a : $(LIBOBJS)
	@echo Building Lib ...
	$(doLib) $@ $(LIBOBJS)

tst: test.o lib
	$(doLink) test.o $(HLIB) -larchive -lcrypto $(BASELIBS) -o $@

clean:
	rm -f *.o *~ core $(TEST) $(DEMO) $(LIBTARGET).a

cppchk:
	cppcheck --template="{file}:{line}:{severity}:{message}" --quiet --force *.c *.h

%.o: %.c
	@echo Compile "$(*F)" ...
	$(doCompile) $(*F).c -o $@

#--------------------------------------------------------
# dependencies
#--------------------------------------------------------

HEADER = db.h common.h dbdict.h

common.o     :  common.c      $(HEADER) common.h
curl.o       :  curl.c        $(HEADER) curl.h
imgtools.o   :  imgtools.c    $(HEADER) imgtools.h
config.o     :  config.c      $(HEADER) config.h
db.o         :  db.c          $(HEADER) db.h
epgservice.o :  epgservice.c  $(HEADER) epgservice.h
dbdict.o     :  dbdict.c      $(HEADER) dbdict.h
json.o       :  json.c        $(HEADER) json.h

demo.o       :  demo.c        $(HEADER)
test.o       :  test.c        $(HEADER)
