#
# Makefile
#
# See the README file for copyright information and how to reach the author.
#
#

PREFIX   = /usr/local
BINDEST  = $(DESTDIR)$(PREFIX)/bin
WEBDEST  = $(DESTDIR)/var/lib/p4
CONFDEST = $(DESTDIR)/etc

DEBUG = 1

# -----------------------
# don't touch below ;)

CC = g++

TARGET = p4d
CMDTARGET = p4
CHARTTARGET = p4chart

LIBS = -lmysqlclient_r -lrt -lcrypto
DEFINES += -D_GNU_SOURCE -DTARGET='"$(TARGET)"'

CFLAGS   = -Wreturn-type -Wall -Wformat -Wunused-variable -Wunused-label \
           -pedantic -Wunused-value -Wunused-function -Wno-long-long \
           -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

ifdef DEBUG
  CFLAGS += -ggdb -O0
endif

VERSION = $(shell grep 'define VERSION ' p4d.h | awk '{ print $$3 }' | sed -e 's/[";]//g')
TMPDIR = /tmp
ARCHIVE = $(TARGET)-$(VERSION)

# object files 

LOBJS =  lib/db.o lib/tabledef.o lib/common.o lib/serial.o
OBJS += $(LOBJS) main.o p4io.o service.o w1.o webif.o
CLOBJS = $(LOBJS) chart.o
CMDOBJS = p4cmd.o p4io.o lib/serial.o service.o w1.o lib/common.o

DEFINES += -DDEAMON=P4d -DUSEMD5
OBJS += p4d.o

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

install: $(TARGET) $(CMDTARGET) install-config install-scripts
	@cp -p $(TARGET) $(CMDTARGET) $(BINDEST)

install-config:
	if ! test -f $(CONFDEST)/p4d.conf; then \
	   install --mode=644 -D ./configs/p4d.conf $(CONFDEST)/; \
	fi

install-scripts:
	if ! test -d $(BINDEST); then \
		mkdir -p "$(BINDEST)" \
	   chmod a+rx $(BINDEST); \
	fi
	install -D ./scripts/p4d-* $(BINDEST)/

install-web:
	if ! test -d $(WEBDEST); then \
		mkdir -p "$(WEBDEST)"; \
		chmod a+rx $(WEBDEST); \
	fi
	if test -f "$(WEBDEST)/config.php"; then \
		cp -p "$(WEBDEST)/config.php" "$(WEBDEST)/config.php.save"; \
	fi
	cp -r ./htdocs/* $(WEBDEST)/
	if test -f "$(WEBDEST)/config.php.save"; then \
		cp -p "$(WEBDEST)/config.php" "$(WEBDEST)/config.php.dist"; \
		cp -p "$(WEBDEST)/config.php.save" "$(WEBDEST)/config.php"; \
	fi
	chmod -R a+r $(WEBDEST)

dist: clean
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@mkdir $(TMPDIR)/$(ARCHIVE)
	@cp -a * $(TMPDIR)/$(ARCHIVE)
	@tar czf $(ARCHIVE).tgz -C $(TMPDIR) $(ARCHIVE)
	@-rm -rf $(TMPDIR)/$(ARCHIVE)
	@echo Distribution package created as $(ARCHIVE).tgz

clean:
	rm -f */*.o *.o core* *~ */*~ lib/t *.jpg
	rm -f $(TARGET) $(CHARTTARGET) $(CMDTARGET) $(ARCHIVE).tgz
	rm -f com2

cppchk:
	cppcheck --template="{file}:{line}:{severity}:{message}" --quiet --force *.c *.h

com2: $(LOBJS) c2tst.c p4io.c service.c
	$(CC) $(CFLAGS) c2tst.c p4io.c service.c $(LOBJS) $(LIBS) -o $@

#***************************************************************************
# dependencies
#***************************************************************************

HEADER = lib/db.h lib/tabledef.h lib/common.h

lib/common.o    :  lib/common.c    $(HEADER)
lib/db.o        :  lib/db.c        $(HEADER)
lib/tabledef.o  :  lib/tabledef.c  $(HEADER)
lib/serial.o    :  lib/serial.c    $(HEADER) lib/serial.h

main.o			 :  main.c          $(HEADER) p4d.h
p4d.o           :  p4d.c           $(HEADER) p4d.h p4io.h w1.h
p4io.o          :  p4io.c          $(HEADER) p4io.h 
webif.o			 :  webif.c         $(HEADER) p4d.h
w1.o			    :  w1.c            $(HEADER) w1.h
service.o       :  service.c       $(HEADER) service.h
chart.o         :  chart.c
