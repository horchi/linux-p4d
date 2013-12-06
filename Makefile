#
# Makefile
#
# See the README file for copyright information and how to reach the author.
#
#

PREFIX   = /usr/local
BINDEST  = $(DESTDIR)$(PREFIX)/bin
CONFDEST = $(DESTDIR)/etc

DEBUG = 1

# -----------------------
# don't touch below ;)

CC = g++

USE_SVC_INTERFACE = 1

TARGET = p4d
CMDTARGET = p4
CHARTTARGET = p4chart
MBTARGET = p4mb

LIBS = -lmysqlclient_r -lrt
DEFINES += -D_GNU_SOURCE -DTARGET='"$(TARGET)"'

CFLAGS   = -Wreturn-type -Wall -Wformat -Wunused-variable -Wunused-label \
           -pedantic -Wunused-value -Wunused-function \
           -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

ifdef DEBUG
  CFLAGS += -ggdb -O0
endif

VERSION = $(shell grep 'define VERSION ' p4sd.h | awk '{ print $$3 }' | sed -e 's/[";]//g')
TMPDIR = /tmp
ARCHIVE = $(TARGET)-$(VERSION)

# object files 

LOBJS =  lib/db.o lib/tabledef.o lib/common.o
OBJS += $(LOBJS) main.o serial.o p4io.o service.o
CLOBJS = $(LOBJS) chart.o
CMDOBJS = p4cmd.o p4io.o serial.o service.o lib/common.o
MBOBJS = mbp4.o serial.o lib/common.o

ifdef USE_SVC_INTERFACE
  DEFINES += -DSVC_INTERFACE -DDEAMON=P4sd
  OBJS += p4sd.o
else
  DEFINES += -DDEAMON=Linpellet
  OBJS += p4d.o
endif

# rules:

%.o: %.c
	$(CC) $(CFLAGS) -c $(DEFINES) -o $@ $<

all: $(TARGET) $(CMDTARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@

$(CHARTTARGET): $(CLOBJS)
	$(CC) $(CFLAGS) $(CLOBJS) $(LIBS) -lmgl -o $@

$(CMDTARGET) : $(CMDOBJS)
	$(CC) $(CFLAGS) $(CMDOBJS) $(LIBS) -o $@

$(MBTARGET) : $(MBOBJS)
	$(CC) $(CFLAGS) $(MBOBJS) $(LIBS) -o $@

install: $(TARGET) install-config
	@cp -p $(TARGET) $(BINDEST)

install-config:
	if ! test -f $(CONFDEST)/p4d.conf; then \
	   install --mode=644 -D ./configs/p4d.conf $(CONFDEST)/; \
	fi

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(ARCHIVE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(ARCHIVE).tgz

clean:
	rm -f */*.o *.o core* *~ */*~ lib/t *.jpg
	rm -f $(TARGET) $(CHARTTARGET) $(CMDTARGET) $(MBTARGET) $(ARCHIVE).tgz

cppchk:
	cppcheck --template="{file}:{line}:{severity}:{message}" --quiet --force *.c *.h

#***************************************************************************
# dependencies
#***************************************************************************

HEADER = lib/db.h lib/tabledef.h lib/common.h p4d.h p4sd.h

lib/common.o    :  lib/common.c      lib/common.h $(HEADER)
lib/config.o    :  lib/config.c      lib/config.h $(HEADER)
lib/db.o        :  lib/db.c          lib/db.h $(HEADER)
lib/tabledef.o  :  lib/tabledef.c    $(HEADER)

main.o			 :  main.c         $(HEADER)
p4d.o           :  p4d.c          $(HEADER)
p4sd.o          :  p4sd.c         $(HEADER)
p4.o            :  p4.c           $(HEADER)
serial.o        :  serial.c       $(HEADER) serial.h
p4io.o          :  p4io.c         $(HEADER) p4io.h 
service.o       :  service.c      $(HEADER) service.h
chart.o         :  chart.c
