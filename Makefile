#
# Makefile
#
# See the README file for copyright information and how to reach the author.
#
#

include Make.config

TARGET = p4d
CMDTARGET = p4
CHARTTARGET = dbchart
HISTFILE  = "HISTORY.h"

LIBS = $(shell mysql_config --libs_r) -lrt -lcrypto -lcurl
LIBS += $(shell xml2-config --libs)

DEFINES += -D_GNU_SOURCE -DTARGET='"$(TARGET)"'

VERSION = $(shell grep 'define _VERSION ' $(HISTFILE) | awk '{ print $$3 }' | sed -e 's/[";]//g')
TMPDIR = /tmp
ARCHIVE = $(TARGET)-$(VERSION)

LASTHIST    = $(shell grep '^20[0-3][0-9]' $(HISTFILE) | head -1)
LASTCOMMENT = $(subst |,\n,$(shell sed -n '/$(LASTHIST)/,/^ *$$/p' $(HISTFILE) | tr '\n' '|'))
LASTTAG     = $(shell git describe --tags --abbrev=0)
BRANCH      = $(shell git rev-parse --abbrev-ref HEAD)
GIT_REV     = $(shell git describe --always 2>/dev/null)

# object files

LOBJS      =  lib/db.o lib/dbdict.o lib/common.o lib/serial.o lib/curl.o
OBJS       = $(LOBJS) main.o p4io.o service.o w1.o webif.o hass.o
MQTTBJS    = lib/mqtt.c
CHARTOBJS  = $(LOBJS) chart.o
CMDOBJS    = p4cmd.o p4io.o lib/serial.o service.o w1.o lib/common.o

CFLAGS    += $(shell mysql_config --include)
CFLAGS    += $(shell xml2-config --cflags)
DEFINES   += -DDEAMON=P4d -DUSEMD5
OBJS      += p4d.o

ifdef MQTTINTERFACE
   OBJS    += $(MQTTBJS)
   LIBS    += -lpaho-mqtt3cs
	DEFINES += -DMQTT_HASS
endif

ifdef GIT_REV
   DEFINES += -DGIT_REV='"$(GIT_REV)"'
endif

# rules:

all: $(TARGET) $(CMDTARGET) $(CHARTTARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $@

$(CHARTTARGET): $(CHARTOBJS)
	$(CC) $(CFLAGS) $(CHARTOBJS) $(LIBS) -o $@

$(CMDTARGET) : $(CMDOBJS)
	$(CC) $(CFLAGS) $(CMDOBJS) $(LIBS) -o $@

install: $(TARGET) $(CMDTARGET) install-config install-scripts
	@cp -p $(TARGET) $(CMDTARGET) $(BINDEST)

inst_rest: $(TARGET) $(CMDTARGET) install-config install-scripts
	/etc/init.d/p4d stop
	@cp -p $(TARGET) $(CMDTARGET) $(BINDEST)
	/etc/init.d/p4d start

inst-sysv-init:
	install --mode=755 -D ./contrib/p4d /etc/init.d/
	install --mode=755 -D ./contrib/runp4d /usr/local/bin/
	update-rc.d p4d defaults

install-config:
	if ! test -d $(CONFDEST); then \
	   mkdir -p $(CONFDEST); \
	   mkdir -p $(CONFDEST)/scripts.d; \
	   chmod a+rx $(CONFDEST); \
	fi
	if ! test -f $(CONFDEST)/p4d.conf; then \
	   install --mode=644 -D ./configs/p4d.conf $(CONFDEST)/; \
	fi
	install --mode=644 -D ./configs/p4d.dat $(CONFDEST)/;

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
	if test -f "$(WEBDEST)/stylesheet.css"; then \
		cp -Pp "$(WEBDEST)/stylesheet.css" "$(WEBDEST)/stylesheet.css.save"; \
	fi
	if test -f "$(WEBDEST)/config.php"; then \
		cp -p "$(WEBDEST)/config.php" "$(WEBDEST)/config.php.save"; \
	fi
	cp -r ./htdocs/* $(WEBDEST)/
	if test -f "$(WEBDEST)/config.php.save"; then \
		cp -p "$(WEBDEST)/config.php" "$(WEBDEST)/config.php.dist"; \
		cp -p "$(WEBDEST)/config.php.save" "$(WEBDEST)/config.php"; \
	fi
	if test -f "$(WEBDEST)/stylesheet.css.save"; then \
		cp -Pp "$(WEBDEST)/stylesheet.css.save" "$(WEBDEST)/stylesheet.css"; \
	fi
	cat ./htdocs/header.php | sed s:"<VERSION>":"$(VERSION)":g > $(WEBDEST)/header.php; \
	chmod -R a+r $(WEBDEST); \
	chown -R $(WEBOWNER):$(WEBOWNER) $(WEBDEST)

install-apache-conf:
	install --mode=644 -D apache2/p4.conf $(APACHECFGDEST)/conf-available/
	rm -f $(APACHECFGDEST)/conf-enabled/p4.conf
	ln -s $(APACHECFGDEST)/conf-available/p4.conf $(APACHECFGDEST)/conf-enabled/p4.conf

install-pcharts:
	if ! test -d $(PCHARTDEST); then \
		git clone https://github.com/bozhinov/pChart2.0-for-PHP7.git $(PCHARTDEST); \
		cd $(PCHARTDEST); \
		git checkout 7.x-compatible; \
		ln -s $(PCHARTDEST) $(WEBDEST)/pChart; \
		chown -R $(WEBOWNER):$(WEBOWNER) $(PCHARTDEST); \
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
	rm -f $(TARGET) $(CHARTTARGET) $(CMDTARGET) $(ARCHIVE).tgz
	rm -f com2

cppchk:
	cppcheck --template="{file}:{line}:{severity}:{message}" --quiet --force *.c *.h

com2: $(LOBJS) c2tst.c p4io.c service.c
	$(CC) $(CFLAGS) c2tst.c p4io.c service.c $(LOBJS) $(LIBS) -o $@

#***************************************************************************
# dependencies
#***************************************************************************

HEADER = lib/db.h lib/dbdict.h lib/common.h

lib/common.o    :  lib/common.c    $(HEADER)
lib/db.o        :  lib/db.c        $(HEADER)
lib/dbdict.o    :  lib/dbdict.c    $(HEADER)
lib/curl.o      :  lib/curl.c      $(HEADER)
lib/serial.o    :  lib/serial.c    $(HEADER) lib/serial.h
lib/mqtt.o      :  lib/mqtt.c      lib/mqtt.h

main.o			 :  main.c          $(HEADER) p4d.h
p4d.o           :  p4d.c           $(HEADER) p4d.h p4io.h w1.h mqtt.h
p4io.o          :  p4io.c          $(HEADER) p4io.h
webif.o			 :  webif.c         $(HEADER) p4d.h
w1.o			    :  w1.c            $(HEADER) w1.h
service.o       :  service.c       $(HEADER) service.h
hass.o          :  hass.c          p4d.h
chart.o         :  chart.c

# ------------------------------------------------------
# Git / Versioning / Tagging
# ------------------------------------------------------

vcheck:
	git fetch
	if test "$(LASTTAG)" = "$(VERSION)"; then \
		echo "Warning: tag/version '$(VERSION)' already exists, update HISTORY first. Aborting!"; \
		exit 1; \
	fi

push: vcheck
	echo "tagging git with $(VERSION)"
	git tag $(VERSION)
	git push --tags
	git push

commit: vcheck
	git commit -m "$(LASTCOMMENT)" -a

git: commit push

showv:
	@echo "Git ($(BRANCH)):\\n  Version: $(LASTTAG) (tag)"
	@echo "Local:"
	@echo "  Version: $(VERSION)"
	@echo "  Change:"
	@echo -n "   $(LASTCOMMENT)"
