include config.mk

inputconfd: inputconfd.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

.PHONY: install
install: inputconfd
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m755 inputconfd $(DESTDIR)$(PREFIX)/bin/

.PHONY: clean
clean:
	rm -f inputconfd
