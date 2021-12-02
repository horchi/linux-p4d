# Makefile
#
# See the README file for copyright information and how to reach the author.
#

include Make.config

HISTFILE = "HISTORY.h"

LIBS += $(shell mysql_config --libs_r) -lrt -lcrypto -lcurl -lpthread -luuid

VERSION      = $(shell grep 'define _VERSION ' $(HISTFILE) | awk '{ print $$3 }' | sed -e 's/[";]//g')
ARCHIVE      = $(TARGET)-$(VERSION)
DEB_BASE_DIR = /root/debs
DEB_DEST     = $(DEB_BASE_DIR)/$(TARGET)-$(VERSION)

LASTHIST     = $(shell grep '^20[0-3][0-9]' $(HISTFILE) | head -1)
LASTCOMMENT  = $(subst |,\n,$(shell sed -n '/$(LASTHIST)/,/^ *$$/p' $(HISTFILE) | tr '\n' '|'))
LASTTAG      = $(shell git describe --tags --abbrev=0)
BRANCH       = $(shell git rev-parse --abbrev-ref HEAD)
GIT_REV      = $(shell git describe --always 2>/dev/null)

# object files

LOBJS        = lib/db.o lib/dbdict.o lib/common.o lib/serial.o lib/curl.o lib/thread.o lib/json.o
MQTTOBJS     = lib/mqtt.o lib/mqtt_c.o lib/mqtt_pal.o
OBJS         = $(MQTTOBJS) $(LOBJS) main.o daemon.o wsactions.o gpio.o hass.o websock.o pysensor.o
OBJS        += p4io.o service.o
CHARTOBJS    = $(LOBJS) chart.o
CMDOBJS      = p4cmd.o p4io.o lib/serial.o service.o lib/common.o

CFLAGS      += $(shell mysql_config --include)
OBJS        += specific.o
W1OBJS       = w1.o lib/common.o lib/thread.o $(MQTTOBJS)

ifdef WIRINGPI
  LIBS += -lwiringPi
endif

ifdef TEST_MODE
	DEFINES += -D__TEST
endif

ifdef GIT_REV
   DEFINES += -DGIT_REV='"$(GIT_REV)"'
endif

# rules:

all: $(TARGET) $(W1TARGET) $(CMDTARGET) $(CHARTTARGET)

$(TARGET) : $(OBJS)
	$(doLink) $(OBJS) $(LIBS) -o $@

$(W1TARGET): $(W1OBJS)
	$(doLink) $(W1OBJS) $(LIBS) -o $@

$(CHARTTARGET): $(CHARTOBJS)
	$(doLink) $(CHARTOBJS) $(LIBS) -o $@

$(CMDTARGET) : $(CMDOBJS)
	$(doLink) $(CMDOBJS) $(LIBS) -o $@

install: $(TARGET) $(W1TARGET) $(CMDTARGET) install-daemon install-web

install-daemon: install-config install-scripts
	@echo install $(TARGET)
   ifneq ($(DESTDIR),)
	   @cp -r contrib/DEBIAN $(DESTDIR)
	   @chown root:root -R $(DESTDIR)/DEBIAN
		sed -i s:"<VERSION>":"$(VERSION)":g $(DESTDIR)/DEBIAN/control
	   @mkdir -p $(DESTDIR)/usr/bin
   endif
	install --mode=755 -D $(TARGET) $(BINDEST)/
	install --mode=755 -D $(W1TARGET) $(BINDEST)/
	install --mode=755 -D $(W1TARGET) $(CMDTARGET) $(BINDEST)/
	make install-systemd
	mkdir -p $(DESTDIR)$(PREFIX)/share/$(TARGET)/
#	install --mode=644 -D arduino/build-nano-atmega328/ioctrl.hex $(DESTDIR)$(PREFIX)/share/$(TARGET)/nano-atmega328-ioctrl.hex

inst_restart: $(TARGET) $(CMDTARGET) install-config install-scripts
	systemctl stop  $(TARGET)
	@cp -p $(TARGET) $(CMDTARGET) $(BINDEST)
	systemctl start $(TARGET)

install-systemd:
	@echo install systemd
	cat contrib/daemon.service | sed s:"<BINDEST>":"$(_BINDEST)":g | sed s:"<AFTER>":"$(INIT_AFTER)":g | sed s:"<TARGET>":"$(TARGET)":g | sed s:"<CLASS>":"$(CLASS)":g |install --mode=644 -C -D /dev/stdin $(SYSTEMDDEST)/$(TARGET).service
	cat contrib/w1mqtt.service | sed s:"<BINDEST>":"$(_BINDEST)":g | sed s:"<AFTER>":"$(INIT_AFTER)":g | install --mode=644 -C -D /dev/stdin $(SYSTEMDDEST)/w1mqtt.service
	chmod a+r $(SYSTEMDDEST)/$(TARGET).service
	chmod a+r $(SYSTEMDDEST)/w1mqtt.service
   ifeq ($(DESTDIR),)
	   systemctl daemon-reload
	   systemctl enable $(TARGET)
   endif

install-config:
	@echo install config
	if ! test -d $(CONFDEST); then \
	   mkdir -p $(CONFDEST); \
	   mkdir -p $(CONFDEST)/scripts.d; \
	   chmod a+rx $(CONFDEST); \
	fi
	install --mode=755 -D ./configs/sysctl $(CONFDEST)/scripts.d
	install --mode=755 -D ./configs/example.sh $(CONFDEST)/scripts.d
	install --mode=755 -D ./configs/sensorExample.py $(CONFDEST)/scripts.d
	if ! test -f $(DESTDIR)/etc/msmtprc; then \
	   install --mode=644 -D ./configs/msmtprc $(DESTDIR)/etc/; \
	fi
	if ! test -f $(CONFDEST)/daemon.conf; then \
	   cat configs/daemon.conf | sed s:"<NAME>":$(NAME):g | install --mode=644 -C -D /dev/stdin $(CONFDEST)/daemon.conf; \
	fi
	install --mode=644 -D ./configs/*.dat $(CONFDEST)/;
	mkdir -p $(DESTDIR)/etc/rsyslog.d
	@cp contrib/rsyslog.conf $(DESTDIR)/etc/rsyslog.d/10-$(TARGET).conf
	mkdir -p $(DESTDIR)/etc/logrotate.d
	@cp contrib/logrotate $(DESTDIR)/etc/logrotate.d/$(TARGET)
	mkdir -p $(DESTDIR)/etc/default
	if ! test -f $(DESTDIR)/etc/default/w1mqtt; then \
	   cp contrib/w1mqtt $(DESTDIR)/etc/default/w1mqtt; \
	fi

install-scripts:
	@echo install scripts
	if ! test -d $(BINDEST); then \
		mkdir -p "$(BINDEST)" \
	   chmod a+rx $(BINDEST); \
	fi
	for f in ./scripts/*.sh; do \
		cp -v "$$f" $(BINDEST)/`basename "$$f"`; \
	done

iw: install-web

install-web:
	@echo install web
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

clean-install:
	rm -rf $(CONFDEST)
	rm -rf $(WEBDEST)
	rm -rf $(SYSTEMDDEST)/$(TARGET).service
	rm -rf $(SYSTEMDDEST)/w1mqtt.service
	rm -rf $(DESTDIR)/etc/default/w1mqtt;
	rm -rf $(DESTDIR)/etc/rsyslog.d/10-$(TARGET).conf
	rm -rf $(DESTDIR)/etc/logrotate.d/$(TARGET)
	rm -rf $(DESTDIR)/etc/default/w1mqtt;
	rm -rf $(BINDEST)/$(TARGET)*
	rm -rf $(BINDEST)/$(W1TARGET)

activate: install
	systemctl restart $(TARGET)
#	tail -f /var/log/$(TARGET).log

cppchk:
	cppcheck --template="{file}:{line}:{severity}:{message}" --language=c++ --force *.c *.h

upload:
	avrdude -q -V -p atmega328p -D -c arduino -b 57600 -P $(AVR_DEVICE) -U flash:w:arduino/build-nano-atmega328/ioctrl.hex:i

build-deb:
	rm -rf $(DEB_DEST)
	make -s install-daemon DESTDIR=$(DEB_DEST) PREFIX=/usr INIT_AFTER=mysql.service
	make -s install-web DESTDIR=$(DEB_DEST) PREFIX=/usr
	dpkg-deb --build $(DEB_BASE_DIR)/$(TARGET)-$(VERSION)

publish-deb:
	echo 'put $(DEB_BASE_DIR)/$(TARGET)-${VERSION}.deb' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:$(TARGET)
	echo 'put contrib/install-deb.sh' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:$(TARGET)
	echo 'rm $(TARGET)-latest.deb' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:$(TARGET)
	echo 'ln -s $(TARGET)-${VERSION}.deb $(TARGET)-latest.deb' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:$(TARGET)
	echo 'chmod 644 $(TARGET)-${VERSION}.deb' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:$(TARGET)
	echo 'chmod 755 install-deb.sh' | sftp -i ~/.ssh/id_rsa2 p7583735@home26485763.1and1-data.host:$(TARGET)

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

main.o          :  main.c          $(HEADER) daemon.h HISTORY.h
daemon.o        :  daemon.c        $(HEADER) daemon.h w1.h lib/mqtt.h websock.h
w1.o            :  w1.c            $(HEADER) w1.h lib/mqtt.h
gpio.o          :  gpio.c          $(HEADER) daemon.h
wsactions.o     :  wsactions.c     $(HEADER) daemon.h
hass.o          :  hass.c          daemon.h
websock.o       :  websock.c       websock.h
pysensor.o      : pysensor.c      $(HEADER) pysensor.h

specific.o      : specific.c      $(HEADER) daemon.h specific.h

p4io.o          :  p4io.c          $(HEADER)
service.o       :  service.c       $(HEADER)
p4cmd.o         :  p4cmd.c         $(HEADER) HISTORY.h
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
