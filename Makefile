#
# Makefile
#
# See the README file for copyright information and how to reach the author.
#

include Make.config

TARGET      = p4d
CMDTARGET   = p4
CHARTTARGET = dbchart
HISTFILE    = "HISTORY.h"

LIBS += $(shell mysql_config --libs_r) -lrt -lcrypto -lcurl -lpthread -luuid

DEFINES += -D_GNU_SOURCE -DTARGET='"$(TARGET)"'

VERSION      = $(shell grep 'define _VERSION ' $(HISTFILE) | awk '{ print $$3 }' | sed -e 's/[";]//g')
ARCHIVE      = $(TARGET)-$(VERSION)
DEB_BASE_DIR = /root/debs
DEB_DEST     = $(DEB_BASE_DIR)/p4d-$(VERSION)

LASTHIST     = $(shell grep '^20[0-3][0-9]' $(HISTFILE) | head -1)
LASTCOMMENT  = $(subst |,\n,$(shell sed -n '/$(LASTHIST)/,/^ *$$/p' $(HISTFILE) | tr '\n' '|'))
LASTTAG      = $(shell git describe --tags --abbrev=0)
BRANCH       = $(shell git rev-parse --abbrev-ref HEAD)
GIT_REV      = $(shell git describe --always 2>/dev/null)

# object files

LOBJS        = lib/db.o lib/dbdict.o lib/common.o lib/serial.o lib/thread.o lib/curl.o lib/json.o
MQTTOBJS     = lib/mqtt.o lib/mqtt_c.o lib/mqtt_pal.o
OBJS         = $(LOBJS) $(MQTTOBJS) main.o p4io.o service.o w1.o hass.o websock.o wsactions.o
CHARTOBJS    = $(LOBJS) chart.o
CMDOBJS      = p4cmd.o p4io.o lib/serial.o service.o w1.o lib/common.o

CFLAGS    	+= $(shell mysql_config --include)
DEFINES   	+= -DDEAMON=P4d
OBJS      	+= p4d.o

ifdef TEST_MODE
	DEFINES += -D__TEST
endif

ifdef GIT_REV
   DEFINES += -DGIT_REV='"$(GIT_REV)"'
endif

# rules:

all: $(TARGET) $(CMDTARGET) $(CHARTTARGET)

$(TARGET) : $(OBJS)
	$(doLink) $(OBJS) $(LIBS) -o $@

$(CHARTTARGET): $(CHARTOBJS)
	$(doLink) $(CHARTOBJS) $(LIBS) -o $@

$(CMDTARGET) : $(CMDOBJS)
	$(doLink) $(CMDOBJS) $(LIBS) -o $@

install: $(TARGET) $(CMDTARGET) install-p4d install-web

install-p4d: install-config install-scripts
   ifneq ($(DESTDIR),)
	   @cp -r contrib/DEBIAN $(DESTDIR)
	   @chown root:root -R $(DESTDIR)/DEBIAN
		sed -i s:"<VERSION>":"$(VERSION)":g $(DESTDIR)/DEBIAN/control
	   @mkdir -p $(DESTDIR)/usr/bin
   endif
	install --mode=755 -D $(TARGET) $(BINDEST)
	install --mode=755 -D $(CMDTARGET) $(BINDEST)
	make install-systemd

inst_restart: $(TARGET) $(CMDTARGET) install-config install-scripts
	systemctl stop p4d
	@cp -p $(TARGET) $(CMDTARGET) $(BINDEST)
	systemctl start p4d

install-systemd:
	cat contrib/p4d.service | sed s:"<BINDEST>":"$(_BINDEST)":g | sed s:"<AFTER>":"$(INIT_AFTER)":g | install --mode=644 -C -D /dev/stdin $(SYSTEMDDEST)/p4d.service
	chmod a+r $(SYSTEMDDEST)/p4d.service
   ifeq ($(DESTDIR),)
	   systemctl daemon-reload
	   systemctl enable p4d
   endif

install-none:

install-config:
	if ! test -d $(CONFDEST); then \
	   mkdir -p $(CONFDEST); \
	   mkdir -p $(CONFDEST)/scripts.d; \
	   chmod a+rx $(CONFDEST); \
	fi
	if ! test -f $(DESTDIR)/etc/msmtprc; then \
	   install --mode=644 -D ./configs/msmtprc $(DESTDIR)/etc/; \
	fi
	if ! test -f $(CONFDEST)/p4d.conf; then \
	   install --mode=644 -D ./configs/p4d.conf $(CONFDEST)/; \
	fi
	mkdir -p $(DESTDIR)/etc/rsyslog.d
	@cp contrib/rsyslog-p4d.conf $(DESTDIR)/etc/rsyslog.d/10-p4d.conf
	mkdir -p $(DESTDIR)/etc/logrotate.d
	@cp contrib/logrotate-p4d $(DESTDIR)/etc/logrotate.d/p4d
	install --mode=644 -D ./configs/p4d.dat $(CONFDEST)/;

install-scripts:
	if ! test -d $(BINDEST); then \
		mkdir -p "$(BINDEST)"; \
	   chmod a+rx $(BINDEST); \
	fi
	install -D ./scripts/p4d-* $(BINDEST)/

iw: install-web

install-web:
	(cd htdocs; $(MAKE) install)

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

build: clean all

activate: install
	systemctl restart $(TARGET)
#	tail -f /var/log/$(TARGET).log

cppchk:
	cppcheck --template="{file}:{line}:{severity}:{message}" --language=c++ --force *.c *.h

com2: $(LOBJS) c2tst.c p4io.c service.c
	$(CPP) $(CFLAGS) c2tst.c p4io.c service.c $(LOBJS) $(LIBS) -o $@

build-deb:
	rm -rf $(DEB_DEST)
	make -s install-p4d DESTDIR=$(DEB_DEST) PREFIX=/usr INIT_AFTER=mysql.service
	make -s install-web DESTDIR=$(DEB_DEST) PREFIX=/usr
	dpkg-deb --build $(DEB_BASE_DIR)/p4d-$(VERSION)

publish-deb:
	echo 'put $(DEB_BASE_DIR)/p4d-${VERSION}.deb' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:p4d
	echo 'put contrib/install-deb.sh' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:p4d
	echo 'rm p4d-latest.deb' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:p4d
	echo 'ln -s p4d-${VERSION}.deb p4d-latest.deb' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:p4d
	echo 'chmod 644 p4d-${VERSION}.deb' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:p4d
	echo 'chmod 755 install-deb.sh' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:p4d

#***************************************************************************
# dependencies
#***************************************************************************

HEADER = lib/db.h lib/dbdict.h lib/common.h p4io.h service.h

lib/common.o    :  lib/common.c    $(HEADER)
lib/db.o        :  lib/db.c        $(HEADER)
lib/dbdict.o    :  lib/dbdict.c    $(HEADER)
lib/curl.o      :  lib/curl.c      $(HEADER)
lib/serial.o    :  lib/serial.c    $(HEADER) lib/serial.h
lib/mqtt.o      :  lib/mqtt.c      lib/mqtt.h lib/mqtt_c.h
lib/mqtt_c.o    :  lib/mqtt_c.c    lib/mqtt_c.h
lib/mqtt_pal.o  :  lib/mqtt_pal.c  lib/mqtt_c.h

main.o          :  main.c          $(HEADER) p4d.h websock.h HISTORY.h
p4d.o           :  p4d.c           $(HEADER) p4d.h w1.h lib/mqtt.h
p4io.o          :  p4io.c          $(HEADER)
w1.o            :  w1.c            $(HEADER) w1.h
service.o       :  service.c       $(HEADER)
hass.o          :  hass.c          p4d.h
websock.o       :  websock.c       $(HEADER) websock.h
wsactions.o     :  wsactions.c     $(HEADER) HISTORY.h
chart.o         :  chart.c

p4cmd.o         :  p4cmd.c         $(HEADER) HISTORY.h

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
