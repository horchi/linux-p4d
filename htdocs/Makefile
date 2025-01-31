
include ../Make.config

linstall:
	make install

install:
	if ! test -L $(WEBDEST); then \
		if ! test -d $(WEBDEST); then \
			mkdir -p "$(WEBDEST)"; \
			chmod a+rx "$(WEBDEST)"; \
		fi ; \
		if test -f "$(WEBDEST)/stylesheet.css"; then \
			cp -Pp "$(WEBDEST)/stylesheet.css" "$(WEBDEST)/stylesheet.css.save"; \
		fi ; \
		cp -r ./* $(WEBDEST)/; \
		if test -f "$(WEBDEST)/stylesheet.css.save"; then \
			cp -Pp "$(WEBDEST)/stylesheet.css.save" "$(WEBDEST)/stylesheet.css"; \
		fi ; \
		chmod -R a+r "$(WEBDEST)"; \
		chown -R $(WEBOWNER):$(WEBOWNER) "$(WEBDEST)"; \
	fi
